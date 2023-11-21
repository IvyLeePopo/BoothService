#include "WebSocketServerClass.h"
#include <QtWebSockets>

static QString getIdentifier(QWebSocket *peer)
{
    return QStringLiteral("%1:%2").arg(peer->peerAddress().toString(),
                                       peer->origin());
}

WebSocketServerClass::WebSocketServerClass(QString serverName, quint16 port):
    m_pWebSocketServer(nullptr)
  ,m_nPort(port)
  ,m_strServerName(serverName)
{

}

WebSocketServerClass::~WebSocketServerClass()
{
    m_pWebSocketServer->close();
    m_pWebSocketServer->deleteLater();
}

void WebSocketServerClass::slotSendToAllClients(const QString &strMessage)
{
    qDebug() << "发送消息: " + strMessage;
    for (QWebSocket *pClient : qAsConst(m_clients)) {
         pClient->sendTextMessage(strMessage);
    }
}

bool WebSocketServerClass::hadClients()
{
    return m_clients.size() > 0;
}

QString WebSocketServerClass::getLocalHostIP()
{
    QString cmd = "ifconfig eth0 | grep 'inet ' | awk '{print $2}'";
    char result[512] = { 0 };
    FILE *fp = nullptr;
    if ((fp = popen(cmd.toLocal8Bit().data(), "r")) != nullptr)
    {
        fgets(result, sizeof(result), fp);
        pclose(fp);
        return QString(result).remove('\n');
    }
    return "";
}

void WebSocketServerClass::slotStartServer()
{
    if(m_pWebSocketServer)
        return;

    m_pWebSocketServer = new QWebSocketServer(m_strServerName, QWebSocketServer::NonSecureMode, this);

    QString strLocalHostIp = getLocalHostIP();
    if (m_pWebSocketServer->listen(QHostAddress(strLocalHostIp), m_nPort))
    {
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &WebSocketServerClass::slotNewConnection);
        qDebug() << m_strServerName << " WebSocketServerClass listening on ip: " << strLocalHostIp << ", port: " << QString::number(m_nPort);
    }
}

void WebSocketServerClass::slotNewConnection()
{
    auto pSocket = m_pWebSocketServer->nextPendingConnection();
    QTextStream(stdout) << getIdentifier(pSocket) << " connected!\n";

    qDebug() << getIdentifier(pSocket) << " connected!\n";

    pSocket->setParent(this);

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServerClass::slotProcessMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServerClass::slotSocketDisconnected);

    m_clients << pSocket;
}

void WebSocketServerClass::slotProcessMessage(const QString &strMessage)
{
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
    for (QWebSocket *pClient : qAsConst(m_clients)) {

        emit sigProcessServerMessage(strMessage);//实际只有一个客户端，

        if (pClient != pSender) //don't echo message back to sender
            pClient->sendTextMessage(strMessage);
    }
}

void WebSocketServerClass::slotSocketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    QTextStream(stdout) << getIdentifier(pClient) << " disconnected!\n";
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}
