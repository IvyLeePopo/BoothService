#ifndef INVOICEPRINTERMANAGER_H
#define INVOICEPRINTERMANAGER_H

#include <QObject>

class InvoicePrinterManager : public QObject
{
    Q_OBJECT

public:
    static InvoicePrinterManager *getInstance();
    void init();

private:
    explicit InvoicePrinterManager(QObject *parent = nullptr);
    static InvoicePrinterManager *self;
};

#endif // InvoicePrinterManager_H
