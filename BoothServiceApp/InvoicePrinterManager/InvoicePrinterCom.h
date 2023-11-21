#ifndef INVOICEPRINTERCOM_H
#define INVOICEPRINTERCOM_H

#include <QObject>

class SerialThread;
class InvoicePrinterCom : public QObject
{
    Q_OBJECT

public:
    static InvoicePrinterCom *getInstance();
    virtual ~InvoicePrinterCom();

public:
    void sendToPrinter(std::string strJson);

private slots:
    void slotRecvOneFrame(QByteArray array);

private:
    explicit InvoicePrinterCom(QObject *parent = nullptr);
    static InvoicePrinterCom *self;

    SerialThread *m_SerialThread;
};

#endif // InvoicePrinterCom_H
