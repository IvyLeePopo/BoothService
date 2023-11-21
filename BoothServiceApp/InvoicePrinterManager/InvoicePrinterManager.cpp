#include "InvoicePrinterManager.h"
#include "WebSocketClientBooth.h"
#include "InvoicePrinterCom.h"

InvoicePrinterManager *InvoicePrinterManager::self = nullptr;
InvoicePrinterManager *InvoicePrinterManager::getInstance()
{
    if (self == nullptr) {
        self = new InvoicePrinterManager();
    }
    return self;
}

InvoicePrinterManager::InvoicePrinterManager(QObject *parent) : QObject(parent)
{
    init();
}

void InvoicePrinterManager::init()
{
    InvoicePrinterCom::getInstance();
}
