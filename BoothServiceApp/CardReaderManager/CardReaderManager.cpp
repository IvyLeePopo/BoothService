#include "CardReaderManager.h"
#include "CardReadWrite.h"

CardReaderManager *CardReaderManager::self = nullptr;
CardReaderManager *CardReaderManager::getInstance()
{
    if (self == nullptr) {
        self = new CardReaderManager();
    }
    return self;
}

CardReaderManager::CardReaderManager(QObject *parent) : QObject(parent)
{
    init();
}

void CardReaderManager::init()
{
    CardReadWrite::getInstance();
}
