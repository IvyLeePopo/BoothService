#include "KeyBoardCom.h"
#include "SerialThread.h"
#include "WebSocketClientBooth.h"
#include "json/json.h"
#include "BaseDefine.h"
#include "QSettingsConfig.h"

KeyBoardCom *KeyBoardCom::self = nullptr;
KeyBoardCom *KeyBoardCom::getInstance()
{
    if (self == nullptr) {
        self = new KeyBoardCom();
    }
    return self;
}

KeyBoardCom::KeyBoardCom(QObject *parent) : QObject(parent)
{
    m_SerialThread = new SerialThread();
    m_SerialThread->start();
    qDebug("串口线程已启动");

    bool ok = m_SerialThread->OpenCom(SETTINGS("KeyBoard", "SerialPortKeyBoard", "ttyUSB0"), SETTINGS("KeyBoard", "BaudrateKeyBoard", 9600),
                                      QSerialPort::Data7, QSerialPort::OddParity, QSerialPort::TwoStop, QSerialPort::NoFlowControl);

    if(!ok)
    {
        qDebug("串口打开失败");
    }
//    else {
//        QByteArray strBeepCmd;
//        if (GlobalConf::getInstance()->getBeep())
//            strBeepCmd = QByteArray::fromRawData("\nB\r", 3);
//        else
//            strBeepCmd = QByteArray::fromRawData("\nb\r", 3);

//        m_SerialThread->WriteCom(strBeepCmd);
//    }

    connect(m_SerialThread, &SerialThread::sigRecvOneFrame, this, &KeyBoardCom::slotRecvOneFrame);
    connect(m_SerialThread, &SerialThread::finished, m_SerialThread, &SerialThread::deleteLater);
}

KeyBoardCom::~KeyBoardCom()
{
    if(nullptr != m_SerialThread)
    {
        m_SerialThread->quit();
        m_SerialThread->wait();
    }

    m_SerialThread = nullptr;
}

void KeyBoardCom::slotRecvOneFrame(QByteArray array)
{
    //std::string strKeyVal = array.toHex(QChar::Space).toUpper().toStdString();
    std::string strKeyVal = array.toHex().toUpper().toStdString();

    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["value"] = strKeyVal;

    std::string strJson = jsonWriter.write(jsonRoot);

    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_KEY_TRIGGER, strJson);
    return;
}
