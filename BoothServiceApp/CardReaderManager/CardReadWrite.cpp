#include "CardReadWrite.h"
#include <QLibrary>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <unistd.h>
#include "WebSocketClientBooth.h"
#include "BaseDefine.h"
#include "KeyAlgorithm.h"
#include "QSettingsConfig.h"

#define READER_STATE_INTERVAL 10 * 1000
#define PSAM_AUTH_INTERVAL 5 * 1000

CardReadWrite *CardReadWrite::self = nullptr;
CardReadWrite *CardReadWrite::getInstance()
{
    if (self == nullptr) {
        self = new CardReadWrite();
    }
    return self;
}

CardReadWrite::CardReadWrite(QObject *parent) : QObject(parent)
{
    m_nPsamSockId = 0;
    m_nHandle = 0;
    m_nCardType = 0;
    m_nPostAuthCnt = 0;

    m_bHasPsamAuthed = false;
    m_bTestMode = SETTINGS("ServerInfo", "TestMode", 1);

    m_strPsamSerialNo = "";
    m_strPsamVersion = "";
    m_strKeyCardType = "";
    m_strTermNo = "";
    m_strRandom = "";

    m_strAreaCode = "";
    m_strCPCID = "";

    m_strFile0015 = "";
    m_strFile0019 = "";
    m_strFile0002 = "";
    m_strCardIssuer = "";
    m_strCardNo = "";

    memset(m_strBlockCardInfo, 0, sizeof(m_strBlockCardInfo));
    memset(m_strM1CardInfo, 0, sizeof(m_strM1CardInfo));
    m_jsonCartList.clear();

    m_pTimerPsamAuth = new QTimer();
    m_pTimerPsamAuth->setSingleShot(false);
    connect(m_pTimerPsamAuth, &QTimer::timeout, this, &CardReadWrite::slotTimerPsamAuth);

    m_pTimerReaderState = new QTimer();
    m_pTimerReaderState->setSingleShot(false);
    connect(m_pTimerReaderState, &QTimer::timeout, this, &CardReadWrite::slotTimerReaderState);

    m_pLibReader = nullptr;
    if (!loadLibrary()) {
        qDebug("动态库加载失败，需要重启程序");
        return;
    }

    if (!openReader()) {
        qDebug("读卡器打开失败，请重新初始化读卡器");
        return;
    }
    else {
        m_pTimerReaderState->start(READER_STATE_INTERVAL);
    }
}

CardReadWrite::~CardReadWrite()
{
    JT_CloseReader(m_nHandle);
    if (m_pTimerPsamAuth)
        delete m_pTimerPsamAuth;

    if (m_pLibReader)
        delete m_pLibReader;
}

bool CardReadWrite::initPsam()
{
    int nProtocolType=0;
    qDebug() << "PSAM SLOT " << QString::number(m_nPsamSockId);

    int nRet = JT_SamReset(m_nHandle, m_nPsamSockId, nProtocolType);
    if (nRet == 0)
    {
        qDebug("PSAM 复位成功");
    }
    else
    {
        qDebug() << "PSAM 复位失败 " << QString::number(nRet);
        return false;
    }

    QString strDir="3F00";
    QString strRsp="";

    char cRsp[100+1] = {0};
    char cTermNo[100+1] = {0};
    char cPsamSerial[100+1] = {0};
    int nLenRsp = 0;
    QString strCmd = QString().asprintf("00A4000002%4s", strDir.toLocal8Bit().data());
    qDebug() << "strCmd = " << strCmd;
    nRet = JT_SamCommand(m_nHandle, m_nPsamSockId, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cRsp, &nLenRsp);
    if (nRet==0)
    {
        qDebug("3F00目录打开成功");
        QString s1(cRsp);
        qDebug() << s1;
        qDebug() << QString::number(nLenRsp);
    }
    else
    {
        qDebug("3F00目录打开失败");
        qDebug() << strRsp;
        return false;
    }

    unsigned char cP2 = 0x15 | 0x80;
    QString strCmdserialmNo = QString().asprintf("00B0%02X%02X%02X", cP2, 0, 14);
    qDebug() << strCmdserialmNo;
    int nRetserialNo = JT_SamCommand(m_nHandle, m_nPsamSockId, strCmdserialmNo.toLocal8Bit().data(), strCmdserialmNo.toLocal8Bit().length(), cPsamSerial, &nLenRsp);
    if (nRetserialNo == 0)
    {
        QString str3(cPsamSerial);
        qDebug() << str3;
        qDebug() << QString::number(nLenRsp);

        QString strSw = str3.right(4);
        qDebug() << strSw;

        if(strSw == "9000")
        {
            qDebug("读取PSAM卡序列号成功");
            m_strPsamSerialNo  = str3.mid(0,20);      //PSAM卡序列号
            qDebug() << m_strPsamSerialNo;

            qDebug("读取PSAM版本号成功");
            m_strPsamVersion  = str3.mid(20,2);      //PSAM卡序列号
            qDebug() << m_strPsamVersion;

            qDebug("读取秘钥卡类型成功");
            m_strKeyCardType  = str3.mid(22,2);      //读取秘钥卡类型成功
            qDebug() << m_strKeyCardType;
        }
    }
    else
    {
        qDebug() << strRsp;
        return false;
    }

    strRsp = "";
    nLenRsp = 0;
    unsigned char cP1 = 0x16 | 0x80;
    QString strCmdTermNo = QString().asprintf("00B0%02X%02X%02X", cP1, 0, 6);
    qDebug() << strCmdTermNo;
    int nRetTermNo = JT_SamCommand(m_nHandle, m_nPsamSockId, strCmdTermNo.toLocal8Bit().data(), strCmdTermNo.toLocal8Bit().length(), cTermNo, &nLenRsp);
    if (nRetTermNo == 0)
    {
        qDebug("读取PSAM卡终端号成功");
        QString str2(cTermNo);
        qDebug() << str2;
        qDebug() << QString::number(nLenRsp);

        QString strSw =str2.right(4);
        qDebug() << strSw;

        if (strSw == "9000")
        {
            qDebug("读取PSAM卡终端号成功!");
            m_strTermNo = str2.left(str2.length()-4);      //终端号
            qDebug() << m_strTermNo;

            if (m_bTestMode)
            {
                qDebug("测试模式，模拟PSAM授权");
                openPsamDF01();
            }
            else
            {
                qDebug("生产模式，开始向服务端申请PSAM授权");
                m_pTimerPsamAuth->start(PSAM_AUTH_INTERVAL);
            }

            return true;
        }
        else
        {
            qDebug("PSAM检测异常");
            return false;
        }
    }
    else
    {
        qDebug("读取PSAM卡终端号失败");
        qDebug() << strRsp;
        return false;
    }
}

bool CardReadWrite::loadLibrary()
{
    QString strLibFile = SETTINGS("ServerInfo", "AppPath", "/home/apps/TwBoothService/");
    switch (SETTINGS("CardReader", "CardReaderType", 0)) {
    case 0: strLibFile += "libReaderApi.so"; break;
    case 1: strLibFile += "libmwReader.so"; break;
    case 2: strLibFile += "ICC_HTXX.so"; break;
    default: strLibFile += "libReaderApi.so"; break;
    }

    if (QFile::exists(strLibFile))
        m_pLibReader = new QLibrary(strLibFile);
    else {
        qDebug() << strLibFile << " don't exists";
        return false;
    }

    if(!m_pLibReader->load())
    {
        qDebug() << strLibFile << " load failed: " << m_pLibReader->errorString();
        return false;
    }
    else
    {
        qDebug() << strLibFile + " load success";
    }

    JT_OpenReader    = reinterpret_cast<TYPE_JT_OpenReader   >(m_pLibReader->resolve("JT_OpenReader"));
    JT_CloseReader   = reinterpret_cast<TYPE_JT_CloseReader  >(m_pLibReader->resolve("JT_CloseReader"));
    JT_OpenCard      = reinterpret_cast<TYPE_JT_OpenCard     >(m_pLibReader->resolve("JT_OpenCard"));
    JT_CloseCard     = reinterpret_cast<TYPE_JT_CloseCard    >(m_pLibReader->resolve("JT_CloseCard"));
    JT_LEDDisplay    = reinterpret_cast<TYPE_JT_LEDDisplay   >(m_pLibReader->resolve("JT_LEDDisplay"));
    JT_AudioControl  = reinterpret_cast<TYPE_JT_AudioControl >(m_pLibReader->resolve("JT_AudioControl"));
    JT_CPUCommand    = reinterpret_cast<TYPE_JT_CPUCommand   >(m_pLibReader->resolve("JT_CPUCommand"));
    JT_SamReset      = reinterpret_cast<TYPE_JT_SamReset     >(m_pLibReader->resolve("JT_SamReset"));
    JT_SamCommand    = reinterpret_cast<TYPE_JT_SamCommand   >(m_pLibReader->resolve("JT_SamCommand"));
    JT_GetStatus     = reinterpret_cast<TYPE_JT_GetStatus    >(m_pLibReader->resolve("JT_GetStatus"));
    JT_GetStatusMsg  = reinterpret_cast<TYPE_JT_GetStatusMsg >(m_pLibReader->resolve("JT_GetStatusMsg"));
    JT_ReaderVersion = reinterpret_cast<TYPE_JT_ReaderVersion>(m_pLibReader->resolve("JT_ReaderVersion"));
    JT_LoadKey       = reinterpret_cast<TYPE_JT_LoadKey      >(m_pLibReader->resolve("JT_LoadKey"));
    JT_ReadBlock     = reinterpret_cast<TYPE_JT_ReadBlock    >(m_pLibReader->resolve("JT_ReadBlock"));
    JT_WriteBlock    = reinterpret_cast<TYPE_JT_WriteBlock   >(m_pLibReader->resolve("JT_WriteBlock"));

    if(!JT_OpenReader)
    {
        qDebug("JT_OpenReader not resolve");
        return false;
    }

    if(!JT_CloseReader)
    {
        qDebug("JT_CloseReader not resolve");
        return false;
    }

    if(!JT_OpenCard)
    {
        qDebug("JT_OpenCard not resolve");
        return false;
    }

    if(!JT_CloseCard)
    {
        qDebug("JT_CloseCard not resolve");
        return false;
    }

    if(!JT_LEDDisplay)
    {
        qDebug("JT_LEDDisplay not resolve");
        return false;
    }

    if(!JT_AudioControl)
    {
        qDebug("JT_AudioControl not resolve");
        return false;
    }

    if(!JT_CPUCommand)
    {
        qDebug("JT_CPUCommand not resolve");
        return false;
    }

    if(!JT_SamReset)
    {
        qDebug("JT_SamReset not resolve");
        return false;
    }

    if(!JT_SamCommand)
    {
        qDebug("JT_SamCommand not resolve");
        return false;
    }

    if(!JT_GetStatus)
    {
        qDebug("JT_GetStatus not resolve");
        return false;
    }

    if(!JT_GetStatusMsg)
    {
        qDebug("JT_GetStatusMsg not resolve");
        return false;
    }

    if(!JT_ReaderVersion)
    {
        qDebug("JT_ReaderVersion not resolve");
        return false;
    }

    if(!JT_LoadKey)
    {
        qDebug("JT_LoadKey not resolve");
        return false;
    }

    if(!JT_ReadBlock)
    {
        qDebug("JT_ReadBlock not resolve");
        return false;
    }

    if(!JT_WriteBlock)
    {
        qDebug("JT_WriteBlock not resolve");
        return false;
    }

    return true;
}

bool CardReadWrite::openReader()
{
    m_nPsamSockId = SETTINGS("CardReader", "PsamSockId", 1);
    m_nHandle = JT_OpenReader(0, const_cast<char*>(SETTINGS("CardReader", "CardReaderCom", "/dev/ttyUSB0").toStdString().c_str()));

    if (m_nHandle <= 0) {
        qDebug() << "打开读卡器失败，错误码: " << QString::number(m_nHandle);
        return false;
    }

    qDebug() << "m_nHandle: " << QString::number(m_nHandle);

    qDebug("初始化PSAM");
    if (initPsam() == false)
    {
        qDebug("初始化PSAM 2 times");
        if (initPsam() == false)
        {
            qDebug("初始化PSAM 3 times");
            if (initPsam() == false)
            {
                qDebug("初始化PSAM 4 times");
                initPsam();
            }
        }
    }

    return true;
}

void CardReadWrite::sendOpenReaderResult()
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    if (openReader()) {
        jsonRoot["errCode"] = 0;  //0成功，其它异常
        jsonRoot["message"] = "读卡器打开成功";

        if (m_pTimerReaderState && !m_pTimerReaderState->isActive())
            m_pTimerReaderState->start(READER_STATE_INTERVAL);
    }
    else {
        jsonRoot["errCode"] = 1;  //0成功，其它异常
        jsonRoot["message"] = "读卡器打开失败";
    }

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_OPEN_READER, strJson);
}

void CardReadWrite::openCard()
{
    //  0 CPU 卡
    //  1 卡片类型为块格式
    //  2 卡片类型为 MAD 格式
    //  3 卡片类型为 PRO 卡 
    //  4 卡片类型为 CPC 卡
    //  5 卡片类型为 DESFIRE 卡（可选）
    //  6 卡片类型为 S70（M1）卡（可选）

    //  其他正值 预留的卡片类型定义值
    //  -1 无卡
    //  -2 打开卡片失败
    //  -100 设备无响应
    //  -1000 传入参数错误
    //  -2000 其它错误

    int nErrCode = 0;
    int nCardPlace;
    int nOpenCardCnt = 0;
    char cPhysicsCardno[100+1] = {0};
    m_nCardType = -1;
    while (m_nCardType < 0 && nOpenCardCnt < 20) {
        m_nCardType = JT_OpenCard(m_nHandle, &nCardPlace, cPhysicsCardno);
        qDebug() << "CardType " << m_nCardType;
        if (nOpenCardCnt++ > 0)
            usleep(50*1000);
    }
    m_strPhysicsCardno = string(cPhysicsCardno);
    switch (m_nCardType) {
    case 0:
        qDebug("ETC卡打开成功");
        break;
    case 1:
    case 99:
        m_nCardType = 1;
        qDebug("身份卡打开成功");
        break;
    case 4:
        qDebug("CPC卡打开成功");
        break;
    case 6:
        qDebug("标签卡打开成功");
        break;
    default:
    {
        nErrCode = 1;
        if (m_nCardType > 0) {
            qDebug() << "读取到意外类型的卡片: " << QString::number(m_nCardType);
        }
        else {
            qDebug() << "打开卡片失败: " << QString::number(m_nCardType);
        }
    }
    break;
    }

    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;  //0成功，其它异常
    jsonRoot["pCardPlace"] = nCardPlace;
    jsonRoot["sPhysicsCardno"] = m_strPhysicsCardno;
    jsonRoot["cardType"] = m_nCardType;

    string strJson = jsonWriter.write(jsonRoot);

    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_OPEN_CARD, strJson);
}

void CardReadWrite::closeCard()
{
    qDebug() << "先关闭卡片: " << JT_CloseCard(m_nHandle);
}

void CardReadWrite::readCpcCard()
{
    QString CPC_DF_EF01 = "";
    QString CPC_DF_EF02 = "";
    QString CPC_DF_EF04 = "";
    QString CPC_MF_EF01 = "";
    QString CPC_MF_EF02 = "";

    if (cpcSelectFile("3F00", 0xEF01) == false) {
        sendReadCpcCardResult(1);
        return;
    }

    if (cpcReadFileContent(0, 30, CPC_MF_EF01))
    {
        qDebug() << "CPC_MF_EF01: " << CPC_MF_EF01;
        if (cpcSelectFile("", 0xEF02) == false) {
            sendReadCpcCardResult(2);
            return;
        }

        if (cpcReadFileContent(0, 64, CPC_MF_EF02))
        {
            qDebug() << "CPC_MF_EF02: " << CPC_MF_EF02;
            if (cpcSelectFile("DF01", 0xEF01) == false) {
                sendReadCpcCardResult(3);
                return;
            }

            if (cpcReadFileContent(0, 128, CPC_DF_EF01))
            {
                qDebug() << "CPC_DF_EF01: " << CPC_DF_EF01;
                if (cpcSelectFile("", 0xEF02) == false) {
                    sendReadCpcCardResult(4);
                    return;
                }

                if (cpcReadFileContent(0, 512, CPC_DF_EF02))
                {
                    qDebug() << "CPC_DF_EF02: " << CPC_DF_EF02;
                    if (cpcSelectFile("", 0xEF04) == false) {
                        sendReadCpcCardResult(5);
                        return;
                    }

                    if (cpcReadFileContent(0, 512, CPC_DF_EF04))
                    {
                        qDebug() << "CPC_DF_EF04: " << CPC_DF_EF04;

                        m_strAreaCode = CPC_MF_EF01.mid(0,8);   //区域代码
                        m_strCPCID    = CPC_MF_EF01.mid(16,16);  //CPCID

                        sendReadCpcCardResult(0, CPC_MF_EF01, CPC_MF_EF02, CPC_DF_EF01, CPC_DF_EF02, CPC_DF_EF04);

                        return;
                    }
                    else
                    {
                        qDebug("CPC_DF_EF04 读取失败");
                        sendReadCpcCardResult(6);
                        return;
                    }
                }
                else
                {
                    qDebug("CPC_DF_EF02 读取失败");
                    sendReadCpcCardResult(7);
                    return;
                }
            }
            else
            {
                qDebug("CPC_DF_EF01 读取失败");
                sendReadCpcCardResult(8);
                return;
            }
        }
        else
        {
            qDebug("CPC_MF_EF02 读取失败");
            sendReadCpcCardResult(9);
            return;
        }
    }
    else
    {
        qDebug("CPC_MF_EF01 读取失败");
        sendReadCpcCardResult(10);
        return;
    }
}

void CardReadWrite::writeCpcCard(string strJson)
{
    if (!m_bHasPsamAuthed && !m_bTestMode) {
        qDebug("PSAM未授权，不允许写卡");
        return;
    }

    Json::Value jsonRoot;
    Json::Reader jsonReader;

    if (!jsonReader.parse(strJson, jsonRoot)) {
        qDebug() << "Json解析失败: " << QString::fromStdString(strJson);
        return;
    }

    QString strDFEF01 = jsonRoot["DFEF01"].asCString();
    QString strDFEF02 = jsonRoot["DFEF02"].asCString();
    QString strDFEF04 = jsonRoot["DFEF04"].asCString();

    if (writeCpcData(0xEF01, strDFEF01) == true) {
        if (writeCpcData(0xEF02, strDFEF02) == true) {
            if (writeCpcData(0xEF04, strDFEF04) == true) {
                sendWriteCpcCardResult(0, "CPC写卡成功");
                return;
            }
        }
    }
}

void CardReadWrite::readCpuCard()
{
    if (openCpuCardDF01())
    {
        qDebug("DF01 打开成功");
        if (readCpuCard0015())
        {
            qDebug("0015 读取成功");
            if (readCpuCard0019())
            {
                qDebug("0019 读取成功");
                if (readCpuCard0002())
                {
                    qDebug("0002 读取成功");
                    sendReadCpuCardResult(0, m_strFile0002, m_strFile0015, m_strFile0019);
                    return;
                }
                else
                {
                    qDebug("0002 读取失败");
                    return;
                }
            }
            else
            {
                qDebug("0019 读取失败");
                return;
            }
        }
        else
        {
            qDebug("0015 读取失败");
            return;
        }
    }
    else
    {
        qDebug("DF01 打开失败");
        return;
    }
}

void CardReadWrite::readM1Card()
{
    m_jsonCartList.clear();
    bool bReadFailed = false;
    int ret = -1;
    int nSecNo = 0;  //扇区号
    int nBlockNo = 0;  //绝对块号
    int nCardCnt = 0;  //记录在标签卡中的卡片数
    QByteArray baCardState;  //卡片状态
    QByteArray baCardBoxNo;  //卡箱号

    Json::Value jsonCart;
    Json::Value jsonSector;
    Json::Value jsonSectorList;
    Json::Value jsonBlock;
    Json::Value jsonBlockList;

    bool bBlockEnd = false;

    memset(m_strM1CardInfo, 0, sizeof(m_strM1CardInfo));

    for (nSecNo = 0; nSecNo < M1_CARD_SECTOR_NUM && bBlockEnd == false; nSecNo++) {
        if (nSecNo < 32) {
            ret = JT_LoadKey(m_nHandle, M1_READ_CARD_KEY_TYPE, nSecNo, const_cast<char*>(M1_CARD_KEY_A));
        }
        else {
            if (nSecNo == 32) {
                //32扇区单独占4个扇区的空间，共128~143十六个block
                ret = JT_LoadKey(m_nHandle, M1_READ_CARD_KEY_TYPE, nSecNo, const_cast<char*>(M1_CARD_KEY_SEC_TAIL));
            }
        }

        if (ret == 0){
            //qDebug(QString::number(nSecNo) + "扇区加载密钥成功");
        }
        else {
            if (nSecNo <= 32)
                qDebug() << QString::number(nSecNo) << "扇区加载密钥失败: " << QString::number(ret);
            continue;
        }

        nBlockNo = nSecNo * BLOCK_CNT_PER_SEC;
        for (int i = nBlockNo; i < nBlockNo + BLOCK_CNT_PER_SEC - 1 && bBlockEnd == false; i++) {
            if (nSecNo < 32) {
                ret = JT_ReadBlock(m_nHandle, M1_READ_CARD_KEY_TYPE, i, m_strM1CardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]);
            }
            else {
                ret = JT_ReadBlock(m_nHandle, M1_READ_CARD_KEY_TYPE, i, m_strM1CardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]);
            }

            if (ret == 0){
                //qDebug(QString::number(i) + "块读取成功: " + string(m_strM1CardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE));

                jsonBlock["blockNum"] = i % BLOCK_CNT_PER_SEC;
                jsonBlock["blockHex"] = string(m_strM1CardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE);
                jsonBlockList.append(jsonBlock);

                if (i == 4) {
                    // 记录着卡片数量的block
                    QByteArray baCardCnt = QByteArray::fromRawData(m_strM1CardInfo[1][0], 4);
                    bool ok;
                    nCardCnt = baCardCnt.toInt(&ok, 16);
                    qDebug() << "标签卡记录的卡片数量: " << QString::number(nCardCnt);
                }

//                if (jsonBlock["blockHex"].asCString() == string("00000000000000000000000000000000")) {
//                    qDebug("读到空数据Block，终止读卡");
//                    bBlockEnd = true;
//                }
            }
            else {
                qDebug() << QString::number(i) << "块读取失败，错误码: " + QString::number(ret);
                bReadFailed = true;
                continue;
            }
        }

        if (jsonBlockList.size() > 0) {
            jsonSector["sectorNum"] = nSecNo;
            jsonSector["blockList"] = jsonBlockList;
            jsonSectorList.append(jsonSector);
            jsonBlockList.clear();
        }
        else {
            qDebug() << nSecNo << "扇区为空，不上传";
        }
    }

    qDebug() << "厂商代码块: " << QByteArray::fromRawData(m_strM1CardInfo[0][0], 32);

    baCardBoxNo = QByteArray::fromRawData(m_strM1CardInfo[0][1] + 10, 10);
    qDebug() << "卡箱号: " + baCardBoxNo;

    baCardState.append(m_strM1CardInfo[0][1][20]).append(m_strM1CardInfo[0][1][21]);
    qDebug() << "卡片状态: " + baCardState;

    if (bReadFailed) {
        qDebug("读取部分扇区失败，需要重新读卡");
        sendReadM1CardResult(1, "读取部分扇区失败，需要重新读卡");
        return;
    }

    jsonCart["cartPosition"] = 0;
    jsonCart["sectorList"] = jsonSectorList;
    m_jsonCartList.append(jsonCart);
    sendReadM1CardResult(0, "读取标签卡成功");

    for (nBlockNo = 0; nBlockNo < M1_CARD_BLOCK_NUM; nBlockNo++) {
        printM1CardInfo(nBlockNo);
    }
}

void CardReadWrite::writeM1Card(string strJson)
{
    Json::Value jsonRoot;
    Json::Reader jsonReader;

    if (!jsonReader.parse(strJson, jsonRoot)) {
        qDebug() << "Json解析失败: " + QString::fromStdString(strJson);
        return;
    }

    if (jsonRoot["cartList"].isArray() && jsonRoot["cartList"].size() > 0) {
        Json::Value jsonCart = jsonRoot["cartList"][static_cast<int>(0)];
        if (jsonCart["cartPosition"].isInt()) {
            qDebug("开始写入标签卡数据");
            Json::Value jsonSector;
            for (uint16_t j = 0; j < jsonCart["sectorList"].size(); j++) {
                jsonSector = jsonCart["sectorList"][j];
//                if (jsonSector["sectorNum"].asInt() == 0) {
//                    qDebug("跳过扇区0不写入");
//                    continue;
//                }

                Json::Value jsonBlock;
                for (uint16_t k = 0; k < jsonSector["blockList"].size(); k++) {
                    jsonBlock = jsonSector["blockList"][k];
                    int nBlockNo = jsonBlock["blockNum"].asInt() + j * BLOCK_CNT_PER_SEC;

//                    if (nBlockNo == 0) {
//                        qDebug("跳过Block0不写入");
//                        continue;
//                    }

                    string strBlockHex = jsonBlock["blockHex"].asCString();
                    if (!writeM1CardBlock(nBlockNo, strBlockHex)) {
                        qDebug() << QString::number(nBlockNo) + "Block写入失败，停止写卡，blockHex: " + QString::fromStdString(strBlockHex);
                        sendWriteM1CardResult(1, "标签卡写入失败，停止写卡");
                        return;
                    }
                }
            }

            qDebug("标签卡写入成功");
            sendWriteM1CardResult(0, "标签卡写入成功");
        }
    }
}

void CardReadWrite::readBlockCard()
{
    bool bReadFailed = false;
    int ret = -1;
    int nSecNo = 0;  //扇区号
    int nBlockNo = 0;  //绝对块号
    QByteArray baUserId;  //员工号

    Json::Value jsonCart;
    Json::Value jsonSector;
    Json::Value jsonSectorList;
    Json::Value jsonBlock;
    Json::Value jsonBlockList;

    memset(m_strBlockCardInfo, 0, sizeof(m_strBlockCardInfo));

    for (nSecNo = 0; nSecNo < BLOCK_CARD_SECTOR_NUM; nSecNo++) {
        if (nSecNo < 1) {
            ret = JT_LoadKey(m_nHandle, BLOCK_CARD_KEY_TYPE, nSecNo, const_cast<char*>(BLOCK_CARD_SEC_0_KEY));
        }
        else {
            ret = JT_LoadKey(m_nHandle, BLOCK_CARD_KEY_TYPE, nSecNo, const_cast<char*>(BLOCK_CARD_SEC_1_KEY));
        }

        if (ret == 0){
            //qDebug(QString::number(nSecNo) + "扇区加载密钥成功");
        }
        else {
            qDebug() << QString::number(nSecNo) << "扇区加载密钥失败: " << QString::number(ret);
            continue;
        }

        nBlockNo = nSecNo * BLOCK_CNT_PER_SEC;
        for (int i = nBlockNo; i < nBlockNo + BLOCK_CNT_PER_SEC - 1; i++) {
            if (nSecNo < 32) {
                ret = JT_ReadBlock(m_nHandle, BLOCK_CARD_KEY_TYPE, i, m_strBlockCardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]);
            }
            else {
                ret = JT_ReadBlock(m_nHandle, BLOCK_CARD_KEY_TYPE, i, m_strBlockCardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]);
            }

            if (ret == 0){
                //qDebug(QString::number(i) + "块读取成功: " + string(m_strBlockCardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE));

                jsonBlock["blockNum"] = i % BLOCK_CNT_PER_SEC;
                jsonBlock["blockHex"] = string(m_strBlockCardInfo[nSecNo][i % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE);
                jsonBlockList.append(jsonBlock);
            }
            else {
                qDebug() << QString::number(i) << "块读取失败，错误码: " << QString::number(ret);
                bReadFailed = true;
                continue;
            }
        }

        jsonSector["sectorNum"] = nSecNo;
        jsonSector["blockList"] = jsonBlockList;
        jsonSectorList.append(jsonSector);
        jsonBlockList.clear();
    }

    baUserId = QByteArray::fromRawData(m_strBlockCardInfo[1][0] + 14, 8);
    qDebug() << "员工号: " << baUserId;

    if (bReadFailed) {
        qDebug("读取部分扇区失败，需要重新读卡");
        sendReadBlockCardResult(1, "读取身份卡部分扇区失败，需要重新读卡");
        return;
    }

    jsonCart["cartPosition"] = 0;
    jsonCart["sectorList"] = jsonSectorList;
    sendReadBlockCardResult(0, "身份卡读取成功", baUserId.toStdString());

    for (nBlockNo = 0; nBlockNo < BLOCK_CARD_BLOCK_NUM; nBlockNo++) {
        printBlockCardInfo(nBlockNo);
    }
}

bool CardReadWrite::cpcSelectFile(QString strDir, int nFileNo)
{
    char cCmdrep[150+1] = {0};
    int nCmdlen=0;

    char cCmdrep1[150+1] = {0};
    int nCmdlen1=0;

    QByteArray baDir = (QString("0000") + strDir).right(4).toLocal8Bit();
    QString strCmd = QString().asprintf("00A4000002%s", baDir.data());
    int nRet = 0;
    if (false == strDir.isEmpty())
    {
        qDebug("-------------------");
        qDebug() << strCmd;
        nRet = JT_CPUCommand(m_nHandle, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cCmdrep, &nCmdlen);
        qDebug() << QString::number(nRet);
        QString cmdRsp(cCmdrep);
        qDebug() << cmdRsp;

        if (cmdRsp.right(4) != "9000") {
            qDebug("cmdRsp尾数不为9000，return false");
            return false;
        }
    }

    if (nRet == 0)
    {
        strCmd = QString().asprintf("00A4000002%04X", nFileNo);
        qDebug() << strCmd;
        nRet = JT_CPUCommand(m_nHandle, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cCmdrep1,&nCmdlen1);
        qDebug() << QString::number(nRet);
        QString cmdRsp1(cCmdrep1);
        qDebug() << cmdRsp1;

        if (cmdRsp1.right(4) != "9000") {
            qDebug("cmdRsp1尾数不为9000，return false");
            return false;
        }
    }

    return true;
}

bool CardReadWrite::cpcReadFileContent(int nOffSet, int nLen, QString &strContent)
{
    int nOffSetTmp = nOffSet;
    unsigned short nP1P2;
    QString strRsp;
    QString strSW12;
    char cCmdRep[150+1] = {0};
    int nCmdLen=0;

    char cCmdRep1[150+1] = {0};
    int nCmdLen1=0;
    while(1)
    {
        if (nOffSetTmp < 0 || nOffSetTmp > (0x0FFF))
        {
            qDebug() << QString::fromLocal8Bit("输入的偏移地址[%1]超出范围（0~0x0FFF）").arg(nOffSetTmp);
            return false;
        }

        nP1P2 = 0x7FFF & nOffSetTmp;
        int nSizeOfRead  = min(nLen - (nOffSetTmp - nOffSet), 0x80);
        QString strCmd = QString().asprintf("00B0%04X%02X", nP1P2, nSizeOfRead);
        qDebug() << strCmd;
        qDebug("-------------------------");
        int nRet = JT_CPUCommand(m_nHandle, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cCmdRep, &nCmdLen);
        qDebug() << QString::number(nRet);
        QString strCmdRsp(cCmdRep);
        qDebug() << strCmdRsp;
        strRsp = strCmdRsp;
        strSW12 = strRsp;
        if (strSW12.indexOf("6C") == 0)
        {
            if (strSW12.indexOf("6C") == 0)
            {
                strSW12 = strSW12.mid(2,2);
                strCmd = strCmd.left(strCmd.length() - 2) + strSW12;

                int nRet1 = JT_CPUCommand(m_nHandle, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cCmdRep1, &nCmdLen1);
                if (nRet1 != 0)
                {
                    qDebug() << QString::fromLocal8Bit("指令:%1，执行失败，SW12:%2").arg(strCmd, strSW12);
                    sendReadCpcCardResult(11);
                    return false;
                }
                else
                {
                    QString strCmdRsp1(cCmdRep1);
                    qDebug() << strCmdRsp1;
                    qDebug() << QString::fromLocal8Bit("指令:%1，执行成功，响应:%2").arg(strCmd, strCmdRsp1);

                    strContent += strCmdRsp1.leftRef(strCmdRsp1.length()-4);//9000去掉
                    break;
                }
            }
            else
            {
                qDebug() << QString::fromLocal8Bit("指令:%1，执行失败，SW12:%2").arg(strCmd,strSW12);
                sendReadCpcCardResult(12);
                return false;
            }
        }
        else
        {
            qDebug() << QString::fromLocal8Bit("指令:%1，执行成功，响应:%2").arg(strCmd, strRsp);
            strContent += strRsp.midRef(0, (strRsp.length() - 4));//9000去掉
            qDebug() << strContent;

            if (strRsp.right(4) != "9000") {
                qDebug("响应串尾四位不为9000，返回false");
                sendReadCpcCardResult(13);
                return false;
            }

            nOffSetTmp += (strRsp.length() / 2) - 2;     //偏移位置：+每次读取字节数
            qDebug("********************************");
            qDebug() << nOffSetTmp;
            if (nOffSetTmp == nLen)
            {
                break;
            }
        }
    }

    qDebug("所有数据............................");
    qDebug() << strContent;
    return true;
}

void CardReadWrite::sendReadCpcCardResult(int nErrCode, QString MFEF01, QString MFEF02, QString DFEF01, QString DFEF02, QString DFEF04)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["MFEF01"] = MFEF01.toStdString();
    jsonRoot["MFEF02"] = MFEF02.toStdString();
    jsonRoot["DFEF01"] = DFEF01.toStdString();
    jsonRoot["DFEF02"] = DFEF02.toStdString();
    jsonRoot["DFEF04"] = DFEF04.toStdString();
    jsonRoot["sPhysicsCardno"] = m_strCPCID.toStdString();

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_CPC_READ, strJson);
    return;
}

void CardReadWrite::sendWriteCpcCardResult(int nErrCode, string strMessage)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["message"] = strMessage;

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_CPC_WRITE, strJson);
    return;
}

bool CardReadWrite::writeCpcData(int nFileNo, QString strContent)
{
    char cCmdRep[150+1] = {0};
    int nCmdLen=0;

    char cCmdRep1[150+1] = {0};
    int nCmdLen1=0;
    char cCmdRep2[150+1] = {0};
    int nCmdLen2=0;

    int nPsam = 1;
    QString strDivFactor;
    if (m_bTestMode) {
        strDivFactor = m_strAreaCode + m_strAreaCode + m_strCPCID;
        nPsam = 0;
    }
    else {
        strDivFactor = m_strCPCID + m_strAreaCode + m_strAreaCode;
        nPsam = 1;
    }

    qDebug() << "m_strAreaCode = " << m_strAreaCode;
    qDebug() << "m_strCPCID = " << m_strCPCID;
    qDebug() << "strDivFactor = " << strDivFactor;

    if (cpcSelectFile("", nFileNo) == false) {
        qDebug("cpcSelectFile failed");
        sendWriteCpcCardResult(1, "cpcSelectFile failed");
        return false;
    }

    QString strWriteCmd = QString::fromLocal8Bit("0084000008");

    int nRet = JT_CPUCommand(m_nHandle, strWriteCmd.toLocal8Bit().data(), strWriteCmd.toLocal8Bit().length(), cCmdRep, &nCmdLen);
    qDebug() << "nRet = " << QString::number(nRet);
    QString strRsp(cCmdRep);
    qDebug() << "strRsp = " + strRsp;
    QString strRandHex = strRsp.left(16) + QString::fromLocal8Bit(QByteArray(32-16,'0'));
    QString strRand;
    qDebug() << QString::fromLocal8Bit("CPC卡随机数：%1").arg(strRandHex);
    if (nPsam == 0)
    {
        QString strKey = "00000000000000000000000000000000";
        if (m_bTestMode) {
            qDebug() << QString::fromLocal8Bit("分散因子：%1").arg(strDivFactor);
            strKey = QString::fromLocal8Bit(TKeyAlgorithm::DiversifyKey_SM4(strDivFactor.toLocal8Bit(), strKey.toLocal8Bit(), 2));
            qDebug() << QString::fromLocal8Bit("两级分散后Key：%1").arg(strKey);
        }

        QString strCzKey = QString::fromLocal8Bit(QByteArray::fromHex(strKey.toLocal8Bit().left(32)));
        qDebug() << strCzKey;

        //SM4加密
        QByteArray baData = TKeyAlgorithm::EncryptWithSM4(strRandHex.toLocal8Bit(), strKey.toLocal8Bit());
        //要用latin1编码，不然后面转出来的字节数组会不对
        strRand = QString::fromLatin1(QByteArray::fromHex(baData.left(32)));
        strRsp = QString::fromLocal8Bit(baData);
        qDebug() << QString::fromLocal8Bit("SM4加密后随机数：%1").arg(strRsp);
    }
    else
    {
        strWriteCmd =  QString::fromLocal8Bit("801A484410%1").arg(strDivFactor);
        qDebug() << strWriteCmd;
        char cDecentralizationrep[150+1] = {0};
        int nDecentralizationlen=0;
        int nCheck = JT_SamCommand(m_nHandle, m_nPsamSockId, strWriteCmd.toLocal8Bit().data(), strWriteCmd.toLocal8Bit().length(), cDecentralizationrep,&nDecentralizationlen);
        qDebug() << QString::number(nCheck);
        QString strDecentralizationrepstrRsp(cDecentralizationrep);
        qDebug() << strDecentralizationrepstrRsp;
        QString strWd = strDecentralizationrepstrRsp.right(4);
        qDebug() << strWd;
        if(strWd != "9000")
        {
            qDebug() << "CPC卡随机数加密失败";
            sendWriteCpcCardResult(5, "CPC卡随机数加密失败");
            return false;
        }

        strWriteCmd =  QString::fromLocal8Bit("80FA000010%1").arg(strRandHex);
        qDebug() << strWriteCmd;
        char cStrRsp1[150+1] = {0};
        int nRspLen=0;
        int nCheck2 = JT_SamCommand(m_nHandle, m_nPsamSockId, strWriteCmd.toLocal8Bit().data(), strWriteCmd.toLocal8Bit().length(), cStrRsp1, &nRspLen);
        qDebug() << QString::number(nCheck2);
        QString strRsp2(cStrRsp1);
        strRsp = strRsp2;
        qDebug() << strRsp;
        if (nRspLen < 32)
        {
            qDebug("加密随机数返回数据长度不足32位");
            sendWriteCpcCardResult(2, "加密随机数返回数据长度不足32位");
            return false;
        }

        strRand = QString::fromLatin1(QByteArray::fromHex(strRsp.toLocal8Bit().left(32)));
        qDebug() << QString::fromLocal8Bit("CPC卡随机数加密结果：%1").arg(strRsp);
    }

    QByteArray baRand = QByteArray::fromHex(strRsp.toLocal8Bit().left(32));
    char cRand[16];
    memcpy(cRand, baRand.data(), 16);
    for(int i = 0; i < 8; i++)
    {
        cRand[i] ^= cRand[8+i];
        cRand[8+i] = 0x00;
    }
    baRand = QByteArray(cRand, 8);
    QString strAuthData = QString::fromLocal8Bit(baRand.toHex()).toUpper();
    qDebug() << QString::fromLocal8Bit("外部认证数据：%1").arg(strAuthData);
    strWriteCmd =  QString::fromLocal8Bit("0082000108%1").arg(strAuthData);
    qDebug() << "strWriteCmd..............................." << strWriteCmd;

    int nRet1 = JT_CPUCommand(m_nHandle, strWriteCmd.toLocal8Bit().data(), strWriteCmd.toLocal8Bit().length(), cCmdRep1, &nCmdLen1);
    qDebug() << QString::number(nRet1);
    QString strRsp1(cCmdRep1);
    qDebug() << strRsp1;

    QString strSw1 = strRsp1.right(4);
    qDebug() << strSw1;
    if(strSw1 == "9000")
    {
        qDebug("外部认证成功!");
        int nOffset=0;
        // 文件多次写入
        qDebug() << QString::fromLocal8Bit("开始写入：%1").arg(strContent);
        const int nCmdMaxLen = 128*2;
        int nContentLen  = strContent.length();
        int nSendTimes = nContentLen/nCmdMaxLen + ((nContentLen%nCmdMaxLen==0)?0:1) ;
        QString strWriteData;
        for(int i=0; i<nSendTimes;i++)
        {
            //写入数据长度大于CmdMaxLen的分多次
            int nCmdLen = (i==nSendTimes-1)? (nContentLen-nCmdMaxLen*i) : nCmdMaxLen ;
            int nP1P2 = 0x7FFF & (i*(nCmdMaxLen/2)+nOffset);      //偏移
            strWriteData = strContent.mid(i*nCmdMaxLen,nCmdLen);

            strWriteCmd = QString().asprintf("00D6%04X%02X",nP1P2, nCmdLen/2) + strWriteData;

            int nRet2 = JT_CPUCommand(m_nHandle, strWriteCmd.toLocal8Bit().data(), strWriteCmd.toLocal8Bit().length(), cCmdRep2, &nCmdLen2);
            qDebug() << QString::number(nRet2);
            QString strRsp2(cCmdRep2);
            qDebug() << strRsp2;
        }

        return true;
    }
    else
    {
        qDebug("外部认证失败!");
        sendWriteCpcCardResult(3, "外部认证失败");
        return false;
    }
}

void CardReadWrite::sendReadCpuCardResult(int nErrCode, QString DF0002, QString DF0015, QString DF0019)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["DF0002"] = DF0002.toStdString();
    jsonRoot["DF0015"] = DF0015.toStdString();
    jsonRoot["DF0019"] = DF0019.toStdString();
    jsonRoot["sPhysicsCardno"] = m_strCardNo.toStdString();
    jsonRoot["psamSerial"] = m_strTermNo.toStdString();
    jsonRoot["psamNo"] = m_strPsamSerialNo.toStdString();

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_CPU_READ, strJson);
    return;
}

bool CardReadWrite::openCpuCardDF01()
{
    qDebug("open DF01");
    QString strDir = "1001"; //"DF01"
    int nDf01len=0;
    char cDf01no[200+1] = {0};
    QString strCmd = QString().asprintf("00A4000002%4s", strDir.toLocal8Bit().data());
    int nCpur1 = JT_CPUCommand(m_nHandle, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cDf01no, &nDf01len);
    if (nCpur1 == 0)
    {
        qDebug("open CPU card df01 file success");
        QString str1(cDf01no);
        qDebug() << str1;
        qDebug() << QString::number(nDf01len);
        return true;
    }
    else
    {
        qDebug("open CPU card df01 file error");
        qDebug() << nCpur1;
        sendReadCpuCardResult(1);
        return false;
    }
}

bool CardReadWrite::readCpuCard0015()
{
    qDebug("open 0015");
    int nCard0015len=0;
    char cCard0015no[150+1] = {0};
    char cSFI=0x15;
    unsigned char cP1 = static_cast<unsigned char>(cSFI | 0x80);
    QString str0015Cmd = QString().asprintf("00B0%02X%02X%02X", cP1, 0, 43);
    qDebug() << str0015Cmd;
    int nCpur2 = JT_CPUCommand(m_nHandle, str0015Cmd.toLocal8Bit().data(), str0015Cmd.toLocal8Bit().length(), cCard0015no, &nCard0015len);
    if (nCpur2 == 0)
    {
        QString str2(cCard0015no);
        m_strFile0015 = str2;
        qDebug() << "m_strFile0015 = " << m_strFile0015;

        QString strSw1 = m_strFile0015.right(4);

        m_strFile0015 = str2.left(str2.length()-4);
        qDebug() << strSw1;
        if(strSw1 == "9000")
        {
            qDebug("获取strFile0015成功!");
            m_strCardIssuer = m_strFile0015.mid(0, 16);   //卡片发行方标识
            m_strCardNo = m_strFile0015.mid(24,16);   //卡片编码
            return true;
        }
        else
        {
            qDebug("获取strFile0015失败!");
            qDebug() << strSw1;
            sendReadCpuCardResult(2);
            return false;
        }
    }
    else
    {
        qDebug("read card 0015 file error!");
        qDebug() << nCpur2;
        sendReadCpuCardResult(3);
        return false;
    }
}

bool CardReadWrite::readCpuCard0019()
{
    int nCard0019len=0;
    char cCard0019no[150+1] = {0};
    unsigned char cP1 = 0xcc;
    QString str0019Cmd = QString().asprintf("00B2%02X%02X%02X", 1, cP1, 43);
    qDebug() << str0019Cmd;
    int nCpur2 = JT_CPUCommand(m_nHandle, str0019Cmd.toLocal8Bit().data(), str0019Cmd.toLocal8Bit().length(), cCard0019no, &nCard0019len);
    if(nCpur2==0)
    {
        QString str2(cCard0019no);
        m_strFile0019 = str2;
        qDebug() << "m_strFile0019 = " << m_strFile0019;

        QString strSw1 = m_strFile0019.right(4);
        qDebug() << strSw1;

        m_strFile0019 = str2.left(str2.length()-4);
        if(strSw1 == "9000")
        {
            qDebug("获取strFile0019成功!");
            return true;
        }
        else
        {
            qDebug("获取strFile0019失败!");
            qDebug() << strSw1;
            sendReadCpuCardResult(4);
            return false;
        }
    }
    else
    {
        qDebug("read card 0019 file error!");
        qDebug() << nCpur2;
        sendReadCpuCardResult(5);
        return false;
    }
}

bool CardReadWrite::readCpuCard0002()
{
    int nCard0002len=0;
    char cCard0002no[20+1] = {0};
    unsigned char cP1 = 0;
    QString str0002Cmd = QString().asprintf("805c%02X%02X%02X", cP1, 2, 4);
    qDebug() << str0002Cmd;
    int nCpur2 = JT_CPUCommand(m_nHandle, str0002Cmd.toLocal8Bit().data(), str0002Cmd.toLocal8Bit().length(), cCard0002no, &nCard0002len);
    if (nCpur2 == 0)
    {
        QString str2(cCard0002no);
        m_strFile0002 = str2;
        qDebug() << "m_strFile0002 = " << m_strFile0002;

        QString strSw1 = m_strFile0002.right(4);
        qDebug() << strSw1;

        m_strFile0002  = str2.left(str2.length()-4);
        if(strSw1 == "9000")
        {
            qDebug("获取strFile0002成功!");
            return true;
        }
        else
        {
            qDebug("获取strFile0002失败!");
            qDebug() << strSw1;
            sendReadCpuCardResult(6);
            return false;
        }
    }
    else
    {
        qDebug("read card 0002 file error");
        qDebug() << nCpur2;
        sendReadCpuCardResult(7);
        return false;
    }
}

void CardReadWrite::writeCpuCard(string strJson)
{
    if (!m_bHasPsamAuthed && !m_bTestMode) {
        qDebug("PSAM未授权，不允许写卡");
        return;
    }

    Json::Value jsonRoot;
    Json::Reader jsonReader;

    if (!jsonReader.parse(strJson, jsonRoot)) {
        qDebug() << "Json解析失败: " << QString::fromStdString(strJson);
        return;
    }

    if (jsonRoot["consumeMoney"].isInt() && jsonRoot["tradeTime"].isString() && jsonRoot["consumeInfo"].isString())
        cpuCardConsume(jsonRoot["consumeMoney"].asInt(), jsonRoot["tradeTime"].asCString(), jsonRoot["consumeInfo"].asCString());
    else {
        qDebug() << "writeCpuCard解析失败: " << QString::fromStdString(strJson);
        return;
    }
}

bool CardReadWrite::cpuCardConsume(unsigned int nConsumeMoney, QString strTradeTime, QString strContent)
{
    char cInitconsume[150+1] = {0};
    int nConsumelen=0;
    char cGetmac1[150+1] = {0};
    int nGetmac1len=0;
    char cWritestion0019[150+1] = {0};
    int nWritestion0019len=0;
    char cConsumecmdrep[150+1] = {0};
    int nConsumecmdreplen=0;

    int nKeyFlag = 1;
    int nTradeType;
    //复合消费初始化
    QString strCmdinit = QString().asprintf("805003020B%02X%08X%12s0F", nKeyFlag, nConsumeMoney, m_strTermNo.toLocal8Bit().data());
    nTradeType = 0x09;
    qDebug() << strCmdinit;
    int nConsumeinit = JT_CPUCommand(m_nHandle, strCmdinit.toLocal8Bit().data(), strCmdinit.toLocal8Bit().length(), cInitconsume, &nConsumelen);
    if (nConsumeinit == 0)
    {
        QString strRsp(cInitconsume);
        qDebug() << strRsp;
        qDebug() << QString::number(nConsumelen);
        QString strSerial = strRsp.mid(8, 4); //交易序号
        QString strKeyVer = QString::asprintf("%02X",nKeyFlag); //密钥版本
        QString strAlgTag = strRsp.mid(20,2); //算法标识
        QString strRandom = strRsp.mid(22,8); //伪随机数
        qDebug() << strSerial;
        qDebug() << strKeyVer;
        qDebug() << strAlgTag;
        qDebug() << strRandom;

        QString strCmdMAC1 = QString().asprintf("8070000024%8s%4s%08X%02X%14s%2s%2s%16s%16s",
                                               strRandom.toLocal8Bit().data(),  //伪随机数
                                               strSerial.toLocal8Bit().data(),  //交易序号
                                               nConsumeMoney,                           //交易金额
                                               nTradeType,                      //交易类型
                                               strTradeTime.toLocal8Bit().data(), //交易时间
                                               strKeyVer.toLocal8Bit().data(),  //密钥版本
                                               strAlgTag.toLocal8Bit().data(),  //算法标识
                                               m_strCardNo.toLocal8Bit().data(),//用户卡号
                                               (m_strCardIssuer.mid(0,8)+m_strCardIssuer.mid(0,8)).toLocal8Bit().data() //发行方
                                               );

        qDebug() << "MAC1 apdu " << strCmdMAC1;

        int nConsumeMAC1 = JT_SamCommand(m_nHandle, m_nPsamSockId, strCmdMAC1.toLocal8Bit().data(), strCmdMAC1.toLocal8Bit().length(), cGetmac1, &nGetmac1len);
        qDebug() << QString::number(nConsumeMAC1);
        QString strMac1Rsp(cGetmac1);

        QString strSw1 = strMac1Rsp.right(4);
        qDebug() << strSw1;
        if (strSw1 == "9000")
        {
            qDebug("获取MAC1成功!");
        }
        else {
            qDebug("获取MAC1失败!");
            sendWriteCpcCardResult(4, "获取MAC1失败");
            return false;
        }
        qDebug() << strMac1Rsp;
        qDebug() << QString::number(nGetmac1len);
        QString strTermTradeSer = strMac1Rsp.mid(0,8); //终端交易序号
        QString strMAC1         = strMac1Rsp.mid(8,8); //MAC1码
        qDebug("终端交易序号 MAC1码");
        qDebug() << strTermTradeSer;
        qDebug() << strMAC1;
        int nWriteLen=strContent.length()/2;
        qDebug() << QString::number(nWriteLen);


        //若是复合消费，更新复合数据缓存0019
        unsigned char cP2 = 0xc8;
        QString strWritestarion = QString().asprintf("80DC%2s%02X%02X%s",
                                                     strContent.mid(0,2).toLocal8Bit().data(),
                                                     cP2,
                                                     nWriteLen,
                                                     strContent.toLocal8Bit().data());
        qDebug() << strWritestarion;

        int nConsumeinit = JT_CPUCommand(m_nHandle, strWritestarion.toLocal8Bit().data(), strWritestarion.toLocal8Bit().length(), cWritestion0019, &nWritestion0019len);
        qDebug() << QString::number(nConsumeinit);
        QString strRsp1(cWritestion0019);
        qDebug() << strRsp1;
        if(strRsp1!="9000")
        {
            qDebug("更新复合数据缓存0019失败");
            sendWriteCpuCardResult(1, "更新复合数据缓存0019失败");
            return false;
        }
        qDebug() << QString::number(nWritestion0019len);

        QString strConsumecmd = QString().asprintf("805401000F%8s%14s%8s08",
                                                   strTermTradeSer.toLocal8Bit().data(),                 //终端交易序号
                                                   strTradeTime.toLocal8Bit().data(), //交易时间
                                                   strMAC1.toLocal8Bit().data());                        //MAC1码
        qDebug() << strConsumecmd;
        int nConsume = JT_CPUCommand(m_nHandle, strConsumecmd.toLocal8Bit().data(), strConsumecmd.toLocal8Bit().length(), cConsumecmdrep, &nConsumecmdreplen);
        qDebug() << QString::number(nConsume);
        QString strRsp2(cConsumecmdrep);
        qDebug() << strRsp2;
        qDebug() << QString::number(nConsumecmdreplen);
        QString strTAC   = strRsp2.mid(0,8); //TAC码
        QString strMAC2  = strRsp2.mid(8,8); //MAC2码
        qDebug() << strTAC;
        qDebug() << strMAC2;
        QString strSw =strRsp2.right(4);
        qDebug() << strSw;

        if (strSw == "9000")
        {
            qDebug("消费成功!");
        }
        else {
            qDebug("消费失败!");
            sendWriteCpuCardResult(2, "消费失败");
            return false;
        }

        QString strCheckMAC2 = QString().asprintf("8072000004%8s", strMAC2.toLocal8Bit().data());
        char cCheckMAC2rep[150+1] = {0};
        int nCheckMAC2len=0;
        int nCheck = JT_SamCommand(m_nHandle, m_nPsamSockId, strCheckMAC2.toLocal8Bit().data(), strCheckMAC2.toLocal8Bit().length(), cCheckMAC2rep, &nCheckMAC2len);
        qDebug() << QString::number(nCheck);
        QString strMac2Rsp(cCheckMAC2rep);
        qDebug() << strMac2Rsp;
        if(strMac2Rsp=="9000")
        {
            bool bOk;
            qDebug() << "原金额: " << QString::number(m_strFile0002.toInt(&bOk, 16));
            readCpuCard0002();
            int nBalance = m_strFile0002.toInt(&bOk, 16);
            qDebug() << "余额:" << QString::number(nBalance);
            qDebug("PSAM卡验证MAC2成功");
            sendWriteCpuCardResult(0, "消费成功", nBalance, strMAC2, strTAC, m_strTermNo, m_strPsamSerialNo, strTermTradeSer);
            return true;
        }
        else {
            qDebug("PSAM卡验证MAC2失败");
            sendWriteCpuCardResult(3, "PSAM卡验证MAC2失败");
            return false;
        }
    }
    else
    {
        qDebug("复合消费初始化失败");
        sendWriteCpuCardResult(4, "复合消费初始化失败");
        return false;
    }
}

void CardReadWrite::sendWriteCpuCardResult(int nErrCode, string strMessage, int Balance, QString MAC2, QString TAC, QString termId, QString termSerialNo, QString tradeSerialNo)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["message"] = strMessage;
    jsonRoot["Balance"] = Balance;
    jsonRoot["MAC2"] = MAC2.toStdString();
    jsonRoot["TAC"] = TAC.toStdString();
    jsonRoot["termId"] = termId.toStdString();
    jsonRoot["termSerialNo"] = termSerialNo.toStdString();
    jsonRoot["tradeSerialNo"] = tradeSerialNo.toStdString();
    jsonRoot["DF0002"] = m_strFile0002.toStdString();

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_CPU_WRITE, strJson);
    return;
}

//将M1卡信息缓存区中指定的数据块写入卡片相应位置
bool CardReadWrite::writeM1CardBlock(int nBlockNo)
{
    if (nBlockNo < 0 || nBlockNo > (M1_CARD_BLOCK_NUM - 1)) {
        qDebug() << "指定的写入块号非法，跳过" << QString::number(nBlockNo);
        return true;
    }

    if (((nBlockNo % BLOCK_CNT_PER_SEC == 3) && nBlockNo < 128) || nBlockNo == 143) {
        qDebug("指定的写入块号为密码块，不允许写入，跳过");
        return true;
    }

    int ret = -1;
    int nSecNo = nBlockNo / BLOCK_CNT_PER_SEC;
    if (nSecNo == 0) {
        qDebug("厂商信息扇区不允许写入，跳过");
        return true;
    }
    else {
        if (nSecNo < 32) {
            ret = JT_LoadKey(m_nHandle, M1_WRITE_CARD_KEY_TYPE, nSecNo, const_cast<char*>(M1_CARD_KEY_B_SEC_OTHER));
        }
        else {
            ret = JT_LoadKey(m_nHandle, M1_READ_CARD_KEY_TYPE, 32, const_cast<char*>(M1_CARD_KEY_SEC_TAIL));
        }
    }

    if (ret == 0) {
        //qDebug(QString::number(nSecNo) + "扇区加载密钥成功");
    }
    else {
        if (nSecNo < 32) {
            qDebug() << QString::number(nSecNo) << "扇区加载密钥失败: " << QString::number(ret);
        }
        else {
            qDebug() << "32扇区加载密钥失败: " << QString::number(ret);
        }
        return false;
    }

    qDebug() << QString::number(nBlockNo) << "块写入卡片: " << QString::fromStdString(string(m_strM1CardInfo[nSecNo][nBlockNo % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE));
    if (nSecNo < 32) {
        ret = JT_WriteBlock(m_nHandle, M1_WRITE_CARD_KEY_TYPE, nBlockNo, const_cast<char*>(string(m_strM1CardInfo[nSecNo][nBlockNo % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE).c_str()));
    }
    else {
        ret = JT_WriteBlock(m_nHandle, M1_READ_CARD_KEY_TYPE, nBlockNo, const_cast<char*>(string(m_strM1CardInfo[nSecNo][nBlockNo % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE).c_str()));
    }

    if (ret == 0) {
        qDebug() << QString::number(nBlockNo) << "块写入成功";
        return true;
    }
    else {
        qDebug() << QString::number(nBlockNo) << "块写入失败，错误码: " << QString::number(ret);
        return false;
    }
}

//将字符串写入卡片指定数据块
bool CardReadWrite::writeM1CardBlock(int nBlockNo, string strData)
{
    if (nBlockNo < 0 || nBlockNo > (M1_CARD_BLOCK_NUM - 1)) {
        qDebug() << "指定的写入块号非法" << QString::number(nBlockNo);
        return false;
    }

    memcpy(m_strM1CardInfo[nBlockNo / BLOCK_CNT_PER_SEC][nBlockNo % BLOCK_CNT_PER_SEC], strData.c_str(), BLOCK_SIZE);
    return writeM1CardBlock(nBlockNo);
}

//打印M1卡信息缓存区中指定的数据块
void CardReadWrite::printM1CardInfo(int nBlockNo)
{
    if (nBlockNo < 0 || nBlockNo > (M1_CARD_BLOCK_NUM - 1)) {
        qDebug() << "指定的打印块号非法" << QString::number(nBlockNo);
        return;
    }

    qDebug() << QString("扇区%1, %2").arg(nBlockNo / BLOCK_CNT_PER_SEC, 2, 10, QChar('0')).arg(nBlockNo, 3, 10, QChar('0')) << "块数据: " << QString::fromStdString(string(m_strM1CardInfo[nBlockNo / BLOCK_CNT_PER_SEC][nBlockNo % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE));
}

void CardReadWrite::printBlockCardInfo(int nBlockNo)
{
    if (nBlockNo < 0 || nBlockNo > (BLOCK_CARD_BLOCK_NUM - 1)) {
        qDebug() << "指定的打印块号非法" << QString::number(nBlockNo);
        return;
    }

    qDebug() << QString("扇区%1, %2").arg(nBlockNo / BLOCK_CNT_PER_SEC, 2, 10, QChar('0')).arg(nBlockNo, 3, 10, QChar('0')) << "块数据: " << QString::fromStdString(string(m_strBlockCardInfo[nBlockNo / BLOCK_CNT_PER_SEC][nBlockNo % BLOCK_CNT_PER_SEC]).substr(0, BLOCK_SIZE));
}

void CardReadWrite::sendReadM1CardResult(int nErrCode, string strMessage)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["message"] = strMessage;
    jsonRoot["cartList"] = m_jsonCartList;

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_CARD_READ, strJson);
    return;
}

void CardReadWrite::sendWriteM1CardResult(int nErrCode, string strMessage)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["message"] = strMessage;

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_CARD_WRITE, strJson);
    return;
}

void CardReadWrite::sendPsamAuthPost()
{
    qDebug("发起PSAM授权申请");
    getRandom();

    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["laneId"] = SETTINGS("ServerInfo", "LaneId", "43017B0278").toStdString();
    jsonRoot["terminalNo"] = m_strTermNo.toStdString();
    jsonRoot["psamNo"] = m_strPsamSerialNo.toStdString();
    jsonRoot["psamVersion"] = m_strPsamVersion.toStdString();
    jsonRoot["keyType"] = m_strKeyCardType.toStdString();
    jsonRoot["areaCode"] = "62010000";
    jsonRoot["random"] = m_strRandom.toStdString();

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_PSAM_AUTH, strJson);
    return;
}

void CardReadWrite::recvPsamAuthPost(string strJson)
{
    Json::Value jsonRoot;
    Json::Reader jsonReader;

    if (!jsonReader.parse(strJson, jsonRoot)) {
        qDebug() << "Json解析失败: " << QString::fromStdString(strJson);
        return;
    }

    if (jsonRoot["authInfo"].isString())
        psamAuth(jsonRoot["authInfo"].asCString());
    else
        qDebug() << "authInfo解析失败: " << QString::fromStdString(strJson);
}

void CardReadWrite::getRandom()
{
    char cGetRandomlc[100+1] = {0};
    int nGetRandomRsp = 0;

    QString strGetRandom = QString::fromLocal8Bit("0084000004");
    qDebug() << strGetRandom;
    int nGetRandomNo = JT_SamCommand(m_nHandle, m_nPsamSockId, strGetRandom.toLocal8Bit().data(), strGetRandom.toLocal8Bit().length(), cGetRandomlc, &nGetRandomRsp);
    if (nGetRandomNo == 0)
    {
        QString str3(cGetRandomlc);
        qDebug() << str3;
        qDebug() << QString::number(nGetRandomRsp);

        QString strSw = str3.right(4);
        qDebug() << strSw;

        if(strSw == "9000")
        {
            qDebug("获取随机数成功!");
            m_strRandom = str3.mid(0,8);
            qDebug() << m_strRandom;
        }
        else {
            qDebug() << "获取随机数失败, strSw = " << strSw;
        }
    }
    else
    {
        qDebug() << "获取随机数失败, nGetRandomNo = " << QString::number(nGetRandomNo);
    }
}

void CardReadWrite::psamAuth(QString authInfo)
{
    qDebug("psamAuth");
    char cAuthlc[100+1] = {0};
    int nAuthlcRsp=0;
    qDebug() << authInfo;

    int nAuthNo = JT_SamCommand(m_nHandle, m_nPsamSockId, authInfo.toLocal8Bit().data(), authInfo.toLocal8Bit().length(), cAuthlc, &nAuthlcRsp);
    if (nAuthNo == 0)
    {
        qDebug() << QString::number(nAuthlcRsp);

        QString strSw(cAuthlc);
        qDebug() << strSw;
        if(strSw == "9000")
        {
            openPsamDF01();
            qDebug("认证成功!");
            qDebug("PSAM授权成功!");
            m_bHasPsamAuthed = true;
            m_pTimerPsamAuth->stop();
        }
        else
        {
            qDebug("PSAM授权失败!");
        }
    }
    else
    {
        qDebug("认证失败!");
    }
}

void CardReadWrite::slotTimerPsamAuth()
{
    if (WebSocketClientBooth::getInstance()->getConnectState()) {
        qDebug() << "申请PSAM授权";
        initPsam();
        sendPsamAuthPost();
        m_nPostAuthCnt++;
    }
    else {
        qDebug() << "未连接服务端，10秒后尝试重新申请PSAM授权";
    }

//    if (m_nPostAuthCnt > ) {
//        m_pTimerPsamAuth->stop();
//        qDebug("授权失败5次，不再申请授权");
//    }
}

void CardReadWrite::slotTimerReaderState()
{
    if (m_nHandle > 0) {
        int nRet;
        int nStatusCode;
        nRet = JT_GetStatus(m_nHandle, &nStatusCode);
        if (nRet == 0) {
            if (nStatusCode != 0) {
                qDebug("读卡器状态异常！");
                if (openReader()) {
                    qDebug("读卡器打开成功，重新授权");

                    if (m_bTestMode)
                    {
                        qDebug("测试模式，模拟PSAM授权");
                        openPsamDF01();
                    }
                    else
                    {
                        qDebug("生产模式，开始向服务端申请PSAM授权");
                        m_bHasPsamAuthed = false;
                        m_pTimerPsamAuth->start(PSAM_AUTH_INTERVAL);
                    }
                }
                else {
                    qDebug("读卡器打开失败");
                }
            }

            if (!m_bTestMode && !m_bHasPsamAuthed) {
                qDebug() << "读卡器状态正常，重新授权";
                m_pTimerPsamAuth->start(PSAM_AUTH_INTERVAL);
            }
        }
        else {
            qDebug("获取读卡器状态失败");
            m_bHasPsamAuthed = false;
        }
    }
}

bool CardReadWrite::openPsamDF01()
{
    QString strDir = "DF01";
    QString strRsp = "";

    char cRsp[200+1] = {0};
    char cApplicationareaIDrec[100+1] = {0};

    int nLenStrRsp = 0;
    QString strCmd = QString().asprintf("00A4000002%4s", strDir.toLocal8Bit().data());
    qDebug() << strCmd;
    int nRet = JT_SamCommand(m_nHandle, m_nPsamSockId, strCmd.toLocal8Bit().data(), strCmd.toLocal8Bit().length(), cRsp, &nLenStrRsp);
    if (nRet == 0)
    {
        qDebug("DF01目录打开成功");
        QString str1(cRsp);
        qDebug() << str1;
        qDebug() << QString::number(nLenStrRsp);

        unsigned char cP2 = 0x17 | 0x80;
        QString strApplicationareaID = QString().asprintf("00B0%02X%02X%02X", cP2, 0, 25);
        qDebug() << strApplicationareaID;
        int nRetSerialNo = JT_SamCommand(m_nHandle, m_nPsamSockId, strApplicationareaID.toLocal8Bit().data(), strApplicationareaID.toLocal8Bit().length(), cApplicationareaIDrec, &nLenStrRsp);
        if (nRetSerialNo == 0)
        {
            QString str3(cApplicationareaIDrec);
            qDebug() << str3;
            qDebug() << QString::number(nLenStrRsp);

            QString strSw =str3.right(4);
            qDebug() << strSw;

            if (strSw == "9000")
            {
                qDebug("读取应用区域标识成功!");
                return true;
            }
            else {
                qDebug("读取应用区域标识失败!");
                return false;
            }
        }
        else
        {
            qDebug() << "读取应用区域标识失败: " + strRsp;
            return false;
        }

        return true;
    }
    else
    {
        qDebug("DF01目录打开失败!");
        qDebug() << strRsp;
        return false;
    }
}

void CardReadWrite::sendReadBlockCardResult(int nErrCode, string strMessage, string userId)
{
    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["errCode"] = nErrCode;
    jsonRoot["message"] = strMessage;
    jsonRoot["userId"] = userId;

    string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_M1_READ, strJson);
    return;
}
