#include "InvoicePrinterCom.h"
#include <QDebug>
#include "SerialThread.h"
#include "WebSocketClientBooth.h"
#include "json/json.h"
#include "BaseDefine.h"
#include "QSettingsConfig.h"

InvoicePrinterCom *InvoicePrinterCom::self = nullptr;
InvoicePrinterCom *InvoicePrinterCom::getInstance()
{
    if (self == nullptr) {
        self = new InvoicePrinterCom();
    }
    return self;
}

InvoicePrinterCom::InvoicePrinterCom(QObject *parent) : QObject(parent)
{
    m_SerialThread = new SerialThread();
    m_SerialThread->start();
    qDebug("串口线程已启动");

    bool ok = m_SerialThread->OpenCom(SETTINGS("Printer", "SerialPortPrinter", "ttyUSB0"), SETTINGS("Printer", "BaudratePrinter", 9600));

    if(!ok)
    {
        qDebug("串口打开失败");
    }

    connect(m_SerialThread, &SerialThread::sigRecvOneFrame, this, &InvoicePrinterCom::slotRecvOneFrame);
    connect(m_SerialThread, &SerialThread::finished, m_SerialThread, &SerialThread::deleteLater);
}

InvoicePrinterCom::~InvoicePrinterCom()
{
    if(nullptr != m_SerialThread)
    {
        m_SerialThread->quit();
        m_SerialThread->wait();
    }

    m_SerialThread = nullptr;
}

void InvoicePrinterCom::slotRecvOneFrame(QByteArray array)
{
    std::string strRsp = array.toHex().toUpper().toStdString();
    qDebug() << "收到打印机响应: " << QString::fromStdString(strRsp);

    Json::Value jsonRoot;
    Json::FastWriter jsonWriter;

    jsonRoot["status"] = 1;  //1成功，其它异常

    std::string strJson = jsonWriter.write(jsonRoot);
    WebSocketClientBooth::getInstance()->sendMessage(BOOTH_WEB_PRINT_MSG, strJson);

    return;
}

void InvoicePrinterCom::sendToPrinter(std::string strJson)
{
    Json::Value jsonRoot;
    Json::Reader jsonReader;

    if (!jsonReader.parse(strJson, jsonRoot)) {
        qDebug() << "Json解析失败: " << QString::fromStdString(strJson);
        return;
    }

    if (jsonRoot["msg"].isString()) {
        QByteArray baPrinterCmd = QByteArray::fromHex(jsonRoot["msg"].asCString());
        m_SerialThread->WriteCom(baPrinterCmd);
    }
    else
        qDebug() << "msg 解析失败: " << QString::fromStdString(strJson);
}
