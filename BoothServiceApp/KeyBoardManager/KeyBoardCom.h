#ifndef KEYBOARDCOM_H
#define KEYBOARDCOM_H

#include <QObject>

class SerialThread;
class KeyBoardCom : public QObject
{
    Q_OBJECT

public:
    static KeyBoardCom *getInstance();
    virtual ~KeyBoardCom();

private slots:
    void slotRecvOneFrame(QByteArray array);

private:
    explicit KeyBoardCom(QObject *parent = nullptr);
    static KeyBoardCom *self;

    SerialThread *m_SerialThread;
};

#endif // KeyBoardCom_H
