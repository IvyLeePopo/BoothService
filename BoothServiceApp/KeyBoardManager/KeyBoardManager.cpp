#include "KeyBoardManager.h"
#include "WebSocketClientBooth.h"
#include "KeyBoardCom.h"

KeyBoardManager *KeyBoardManager::self = nullptr;
KeyBoardManager *KeyBoardManager::getInstance()
{
    if (self == nullptr) {
        self = new KeyBoardManager();
    }
    return self;
}

KeyBoardManager::KeyBoardManager(QObject *parent) : QObject(parent)
{
    init();
}

void KeyBoardManager::init()
{
    KeyBoardCom::getInstance();
}
