#include "WebSocketClientBooth.h"
#include <QTimer>
#include <QThread>
#include <QDateTime>
#include <QThread>
#include <QUuid>
#include <QDebug>
#include "json/json.h"
#include "CardReadWrite.h"
#include "InvoicePrinterCom.h"
#include "QSettingsConfig.h"
#include "WebSocketClientClass.h"
#include "BaseDefine.h"

WebSocketClientBooth *WebSocketClientBooth::self = nullptr;
WebSocketClientBooth *WebSocketClientBooth::getInstance()
{
    if (self == nullptr) {
        self = new WebSocketClientBooth();
    }
    return self;
}

WebSocketClientBooth::WebSocketClientBooth()
{
    m_thdKeyBoardClient = new QThread();
    m_wsKeyBoardClient = new WebSocketClientClass();
    m_wsKeyBoardClient->setWebSocketUrl(SETTINGS("ServerInfo", "ServerURL", "ws://192.168.13.62:10090/edge-modules-booth/booth/ws?appId=43017B0278"));
    m_wsKeyBoardClient->setWebSocketType(0);

    m_wsKeyBoardClient->moveToThread(m_thdKeyBoardClient);
    connect(m_thdKeyBoardClient, &QThread::started, m_wsKeyBoardClient, &WebSocketClientClass::slotCreateDataRecWS);
    connect(m_wsKeyBoardClient, &WebSocketClientClass::sigClientTextMessageReceived, this, &WebSocketClientBooth::slotRecvServerData);
    connect(this, &WebSocketClientBooth::sigSendToServer, m_wsKeyBoardClient, &WebSocketClientClass::slotSendTextMessage);
    connect(this, &WebSocketClientBooth::sigReconnectServer, m_wsKeyBoardClient, &WebSocketClientClass::slotActiveReconnect);

    connect(m_thdKeyBoardClient, &QThread::finished, m_wsKeyBoardClient, &WebSocketClientClass::deleteLater);
    connect(m_thdKeyBoardClient, &QThread::finished, m_thdKeyBoardClient, &QThread::deleteLater);
    m_thdKeyBoardClient->start();

    m_pTimerKeyBoard = new QTimer(this);
    connect(m_pTimerKeyBoard, &QTimer::timeout, this, &WebSocketClientBooth::slotHeartBeatToServer);
    m_nHeartBeatTimeOutKeyBoard = 0;
    m_pTimerKeyBoard->start(10*1000);
}

WebSocketClientBooth::~WebSocketClientBooth()
{
    m_thdKeyBoardClient->quit();
    m_thdKeyBoardClient->wait();

    if (m_wsKeyBoardClient)
        delete m_wsKeyBoardClient;

    if (m_pTimerKeyBoard)
        delete m_pTimerKeyBoard;
}

void WebSocketClientBooth::slotRecvServerData(const QString &strMessage)
{
    m_nHeartBeatTimeOutKeyBoard = 0;

    Json::Value jsonRoot;
    Json::Reader jsonReader;

    if (!jsonReader.parse(strMessage.toStdString(), jsonRoot)) {
        qDebug() << "Json解析失败: " << strMessage;
        return;
    }

    const string strApiKey = jsonRoot["apiKey"].asCString();

    if (BOOTH_KEY_HEART == strApiKey) {
        qDebug("接收[服务端]心跳");
        return;
    }

    if (BOOTH_KEY_TRIGGER == strApiKey) {
        qDebug("接收[服务端]按键回应");
        return;
    }

    m_hashMsgId.insert(QString::fromStdString(strApiKey), QString(jsonRoot["msgId"].asCString()));
    if (BOOTH_WEB_OPEN_READER == strApiKey) {
        qDebug() << "接收[服务端]打开读卡器命令: " << strMessage;
        CardReadWrite::getInstance()->sendOpenReaderResult();
    }
    else if (BOOTH_WEB_OPEN_CARD == strApiKey) {
        qDebug() << "接收[服务端]打开卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
    }
    else if (BOOTH_WEB_CPC_READ == strApiKey) {
        qDebug() << "接收[服务端]读取CPC卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        if (CardReadWrite::getInstance()->getCardType() != -1) {
            CardReadWrite::getInstance()->readCpcCard();
        }
        else {
            CardReadWrite::getInstance()->sendReadCpcCardResult(14);
        }
    }
    else if (BOOTH_WEB_CPC_WRITE == strApiKey) {
        qDebug() << "接收[服务端]写入CPC卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        if (CardReadWrite::getInstance()->getCardType() != -1) {
            CardReadWrite::getInstance()->readCpcCard();
            CardReadWrite::getInstance()->writeCpcCard(jsonRoot["bizContent"].asCString());
        }
        else {
            CardReadWrite::getInstance()->sendWriteCpcCardResult(6, "未寻到CPC卡");
        }
    }
    else if (BOOTH_WEB_CPU_READ == strApiKey) {
        qDebug() << "接收[服务端]读取CPU卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        CardReadWrite::getInstance()->readCpuCard();
    }
    else if (BOOTH_WEB_CPU_WRITE == strApiKey) {
        qDebug() << "接收[服务端]写入CPU卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        CardReadWrite::getInstance()->readCpuCard();
        CardReadWrite::getInstance()->writeCpuCard(jsonRoot["bizContent"].asCString());
    }
    else if (BOOTH_WEB_CARD_READ == strApiKey) {
        qDebug() << "接收[服务端]读取M1卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        CardReadWrite::getInstance()->readM1Card();
    }
    else if (BOOTH_WEB_CARD_WRITE == strApiKey) {
        qDebug() << "接收[服务端]写入M1卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        CardReadWrite::getInstance()->readM1Card();
        CardReadWrite::getInstance()->writeM1Card(jsonRoot["bizContent"].asCString());
    }
    else if (BOOTH_WEB_M1_READ == strApiKey) {
        qDebug() << "接收[服务端]读取Block卡片命令: " << strMessage;
        CardReadWrite::getInstance()->closeCard();
        CardReadWrite::getInstance()->openCard();
        CardReadWrite::getInstance()->readBlockCard();
    }
    else if (BOOTH_PSAM_AUTH == strApiKey) {
        qDebug() << "接收[服务端]PSAM授权申请响应: " << strMessage;
        CardReadWrite::getInstance()->recvPsamAuthPost(jsonRoot["bizContent"].asCString());
    }
    else if (BOOTH_WEB_PRINT_MSG == strApiKey) {
        qDebug() << "接收[服务端]打印机命令: " << strMessage;
        InvoicePrinterCom::getInstance()->sendToPrinter(jsonRoot["bizContent"].asCString());
    }
    else {
        qDebug() << "接收[服务端]返回未定义数据：" << strMessage;
    }

    return;
}

void WebSocketClientBooth::slotHeartBeatToServer()
{
    sendMessage(BOOTH_KEY_HEART, "{\"status\":1}");
    m_nHeartBeatTimeOutKeyBoard++;

    if(m_nHeartBeatTimeOutKeyBoard > 3)
    {
        qDebug("未收到服务端的心跳应答，开启websocket主动重连...");
        emit sigReconnectServer();
        m_nHeartBeatTimeOutKeyBoard = 0;
        return;
    }

    return;
}

void WebSocketClientBooth::sendMessage(const std::string &strApiKey, const std::string &strBizContent)
{
    QString strKey = QString::fromStdString(strApiKey);
    QString strValue = "";
    if (m_hashMsgId.contains(strKey)) {
        strValue = m_hashMsgId[strKey];
        m_hashMsgId.remove(strKey);
    }
    else {
        if (strApiKey == BOOTH_WEB_OPEN_CARD ||
            strApiKey == BOOTH_WEB_CPC_READ ||
            strApiKey == BOOTH_WEB_CPU_READ ||
            strApiKey == BOOTH_WEB_CARD_READ)
        {
            qDebug() << "主动发起的命令，不需要回应: " << QString::fromStdString(strApiKey);
            return;
        }
    }

    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["msgId"] = !strValue.isEmpty() ? strValue.toStdString() : QUuid::createUuid().toString().remove("{").remove("}").remove(" ").remove("-").mid(0,32).toStdString();
    jsonRoot["apiKey"] = strApiKey;
    jsonRoot["time"] = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss").toStdString();
    jsonRoot["version"] = "1.0";
    jsonRoot["bizType"] = "JSON";
    jsonRoot["sign"] = "NONE";
    jsonRoot["bizContent"] = strBizContent;

    emit sigSendToServer(QString::fromStdString(jsonWriter.write(jsonRoot)));
}

bool WebSocketClientBooth::getConnectState()
{
    return m_wsKeyBoardClient->m_bConnectStatus;
}
