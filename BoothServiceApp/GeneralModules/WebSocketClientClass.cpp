#include "WebSocketClientClass.h"
#include <QWebSocket>
#include <QTimer>
#include "BaseDefine.h"

WebSocketClientClass::WebSocketClientClass(QObject *parent) : QObject(parent)
  ,m_bConnectStatus(false)
  ,m_pTimer(nullptr)
  ,m_pDataRecvWS(nullptr)
  ,m_strURL("")
  ,m_nWsType(0)
{

}

WebSocketClientClass::~WebSocketClientClass()
{
    m_pTimer->stop();
    m_pTimer->deleteLater();
    m_pDataRecvWS->abort();
    m_pDataRecvWS->deleteLater();
}

void WebSocketClientClass::setWebSocketUrl(QString strUrl)
{
    m_strURL = strUrl;
    if(m_strURL.isEmpty())
    {
        m_strURL = "127.0.0.1";
    }
}

void WebSocketClientClass::setWebSocketType(int iWsType)
{
    m_nWsType = iWsType;
}

void WebSocketClientClass::slotCreateDataRecWS()
{
    if(nullptr == m_pTimer)
    {
        m_pTimer = new QTimer();
    }

    qDebug() << "连接服务器，地址：" << m_strURL;

    if(m_pDataRecvWS == nullptr)
    {
        m_pDataRecvWS = new QWebSocket();
        connect(m_pDataRecvWS, &QWebSocket::disconnected, this, &WebSocketClientClass::slotOnDisConnected);
        connect(m_pDataRecvWS, &QWebSocket::textMessageReceived, this, &WebSocketClientClass::slotOnTextReceived);
        connect(m_pDataRecvWS, &QWebSocket::connected, this, &WebSocketClientClass::slotOnConnected);
        connect(m_pTimer, &QTimer::timeout, this, &WebSocketClientClass::slotReconnect);
        m_pDataRecvWS->open(QUrl(m_strURL));
    }
}

void WebSocketClientClass::slotSendTextMessage(const QString &message)
{
    if (m_pDataRecvWS)
    {
        m_pDataRecvWS->sendTextMessage(message);
        if (0 == m_nWsType)
        {
            if (message.contains(BOOTH_KEY_HEART)) {
                qDebug() << "向[服务端]发送心跳";
            }
            else {
                qDebug() << "向[服务端]发送数据:  " << message;
            }
        }
    }

    return;
}


void WebSocketClientClass::slotActiveReconnect()
{
    qDebug("try to Active Reconnect!!!");
    if(m_pDataRecvWS != nullptr)
    {
        m_bConnectStatus = false;
        m_pDataRecvWS->abort();
        qDebug("Exec Active Reconnect!");
        m_pDataRecvWS->open(QUrl(m_strURL));
    }

    return;
}

void WebSocketClientClass::slotReconnect()
{
    qDebug() << "try to reconnect " << m_strURL;

    m_pDataRecvWS->abort();
    m_pDataRecvWS->open(QUrl(m_strURL));
}

void WebSocketClientClass::slotOnConnected()
{
    qDebug("WebSocketClientClass websocket is already connect!");

    m_bConnectStatus = true;
    m_pTimer->stop();
    qDebug() << "Address：" << m_strURL;
}

void WebSocketClientClass::slotOnDisConnected()
{
    qDebug() << "Address is disconnected: " << m_strURL;

    m_bConnectStatus = false;
    m_pTimer->start(3000);/*-<当连接失败时，触发重连计时器，设置计数周期为3秒 */
}


void WebSocketClientClass::slotOnTextReceived(const QString& msg)
{
    emit sigClientTextMessageReceived(msg);
}
