#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

#include <QObject>

class KeyBoardManager : public QObject
{
    Q_OBJECT

public:
    static KeyBoardManager *getInstance();

public:
    void init();

private:
    explicit KeyBoardManager(QObject *parent = nullptr);
    static KeyBoardManager *self;
};

#endif // KeyBoardManager_H
