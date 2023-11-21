#ifndef WEBSOCKETCLIENTBOOTH_H
#define WEBSOCKETCLIENTBOOTH_H

#include <QObject>
#include <QHash>

class WebSocketClientClass;
class QTimer;
class WebSocketClientBooth : public QObject
{
    Q_OBJECT

public:
    static WebSocketClientBooth *getInstance();
    virtual ~WebSocketClientBooth();

public:
    //将数据打包成外场接收格式并发送
    void sendMessage(const std::string &strApiKey, const std::string &strBizContent);

    bool getConnectState();

signals:
    //发送文本
    void sigSendToServer(const QString &strMessage);

    //发起客户端重连信号
    void sigReconnectServer();

private slots:
    //接收客户端数据
    void slotRecvServerData(const QString &strMessage);

    //发送客户端心跳包
    void slotHeartBeatToServer();

private:
    WebSocketClientBooth();
    static WebSocketClientBooth *self;

private:
    WebSocketClientClass* m_wsKeyBoardClient;
    QThread* m_thdKeyBoardClient;
    QTimer *m_pTimerKeyBoard;
    int m_nHeartBeatTimeOutKeyBoard;

    QHash<QString, QString> m_hashMsgId;
};


#endif // WEBSOCKETCLIENTBOOTH_H
