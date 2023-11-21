#ifndef CARDREADERMANAGER_H
#define CARDREADERMANAGER_H

#include <QObject>

class CardReaderManager : public QObject
{
    Q_OBJECT

public:
    static CardReaderManager *getInstance();
    void init();

private:
    explicit CardReaderManager(QObject *parent = nullptr);
    static CardReaderManager *self;
};

#endif // CardReaderManager_H
