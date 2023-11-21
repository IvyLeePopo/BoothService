#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QMutex>
#include <unistd.h>
#include <sys/syscall.h>
#include "WebSocketClientBooth.h"
#include "KeyBoardManager.h"
#include "CardReaderManager.h"
#include "InvoicePrinterManager.h"
#include "QSettingsConfig.h"

static QMutex g_qmutex;
int level = 4;
bool checkLogThreadFlag = true;
int logTimeInterval = 600;
long long fileOffSet=0;

void outputMessage(QtMsgType type, const QMessageLogContext& Context, const QString &data)
{
    Q_UNUSED(Context);

    if(g_qmutex.tryLock(10)==false)
    {
        return;
    }

    QString msg(data);
    QString text;
    int outputLevel = 0;
    switch(type)
    {
    case QtDebugMsg:
    {
        text = QString("Debug:");
        outputLevel = 4;
        break;
    }
    case QtWarningMsg:
    {
        text = QString("Warning:");
        outputLevel = 3;
        break;
    }
    case QtCriticalMsg:
    {
        text = QString("Critical:");
        outputLevel = 2;
        break;
    }
    case QtFatalMsg:
    {
        text = QString("Fatal:");
        outputLevel = 1;
        break;
    }
    case QtInfoMsg:
    {
        text = QString("Info");
        outputLevel = 0;
        break;
    }
    }

    if (level < outputLevel)
    {
        return;
    }

    QString current_date_time = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss.zzz");
    QString current_date = QString("[%1]").arg(current_date_time);
    QString message = QString("%1 %2").arg(current_date, msg);

    QString file_time = QDate::currentDate().toString("yyyyMMdd");

    QString file_name1  = QObject::tr("/opt/log/LogBoothService%1.log").arg(file_time);

    QFile file(file_name1);
    file.open(QIODevice::ReadWrite | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << message << "\r\n";
    file.flush();
    file.close();
    g_qmutex.unlock();
}

static void handleLog(QString strLogPath, QString strLogName, int nExpireDays)
{
    QString strCmd;
    QString strBeforeDay = QDateTime::currentDateTime().addDays(-1).toString("yyyyMMdd");
    QString strExpiredDay = QDateTime::currentDateTime().addDays(-nExpireDays).toString("yyyyMMdd");
    QString strBeforeDayLogFileName, strBeforeDayTarFileName;
    QString strExpiredDayLogFileName, strExpiredDayTarFileName;
    QFile fileBeforeDayLog, fileBeforeDayTar, fileExpiredDayLog, fileExpiredDayTar;
    strBeforeDayLogFileName = QObject::tr("%1/%2%3.log").arg(strLogPath, strLogName, strBeforeDay);
    strBeforeDayTarFileName = QObject::tr("%1/%2%3.tar.gz").arg(strLogPath, strLogName, strBeforeDay);
    qDebug() << "[tarLogThread]" << "开始检查前一天日志是否存在: " << strBeforeDayLogFileName;
        fileBeforeDayLog.setFileName(strBeforeDayLogFileName);
    fileBeforeDayTar.setFileName(strBeforeDayTarFileName);
    if (fileBeforeDayLog.exists())
    {
        if (!fileBeforeDayTar.exists()) {
            strCmd = QObject::tr("tar zcvfP %1 %2").arg(strBeforeDayTarFileName, strBeforeDayLogFileName);
            qDebug() << "[tarLogThread]" << "前一天日志存在，且尚未压缩，执行压缩指令: " << strCmd;
            system(strCmd.toLocal8Bit().data());
            qDebug() << "[tarLogThread]" << strBeforeDayTarFileName << "压缩完成，删除源文件";
        }

        if (fileBeforeDayLog.remove()) {
            qDebug() << "[tarLogThread]" << strBeforeDayLogFileName << "删除成功";
        }
        else {
            qDebug() << "[tarLogThread]" << strBeforeDayLogFileName << "删除失败";
        }
    }

    strExpiredDayLogFileName = QObject::tr("%1/%2%3.log").arg(strLogPath, strLogName, strExpiredDay);
    fileExpiredDayLog.setFileName(strExpiredDayLogFileName);
    if (fileExpiredDayLog.exists()) {
        qDebug() << "[tarLogThread]" << strExpiredDayLogFileName << "日志文件已经超过" << nExpireDays << "天，执行删除";
            if (fileExpiredDayLog.remove()) {
            qDebug() << "[tarLogThread]" << strExpiredDayLogFileName << "删除成功";
        }
        else {
            qDebug() << "[tarLogThread]" << strExpiredDayLogFileName << "删除失败";
        }
    }

    strExpiredDayTarFileName = QObject::tr("%1/%2%3.tar.gz").arg(strLogPath, strLogName, strExpiredDay);
    fileExpiredDayTar.setFileName(strExpiredDayTarFileName);
    if (fileExpiredDayTar.exists()) {
        qDebug() << "[tarLogThread]" << strExpiredDayTarFileName << "日志压缩文件已经超过" << nExpireDays << "天，执行删除";
            if (fileExpiredDayTar.remove()) {
            qDebug() << "[tarLogThread]" << strExpiredDayTarFileName << "删除成功";
        }
        else {
            qDebug() << "[tarLogThread]" << strExpiredDayTarFileName << "删除失败";
        }
    }
}

static void* tarLogThread(void*)
{
    qDebug() << "[tarLogThread]" << "日志压缩线程启动...";
    while (true) {
        sleep(static_cast<unsigned int>(SETTINGS("LogOption", "interval", 600)));
        if (SETTINGS("LogOption", "start", 1)) {
            QString strLogPath = SETTINGS("LogOption", "path", "/opt/log");
            int nExpireDays = SETTINGS("LogOption", "days", 40);
            handleLog(strLogPath, "LogBoothService", nExpireDays);
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QDir dirLog;
    if (!dirLog.exists("/opt/log"))
        dirLog.mkpath("/opt/log");

    qInstallMessageHandler(outputMessage);

    qDebug() << "程序发布时间:" << QString(__DATE__) << QString(__TIME__);
    qDebug() << "程序版本 Version: " << APP_VERSION;

    WebSocketClientBooth::getInstance();

    if (SETTINGS("CardReader", "CardReaderOpen", 0))
        CardReaderManager::getInstance();

    if (SETTINGS("KeyBoard", "KeyBoardOpen", 0))
        KeyBoardManager::getInstance();

    if (SETTINGS("Printer", "PrinterOpen", 0))
        InvoicePrinterManager::getInstance();

    pthread_t tidTarLog;
    pthread_create(&tidTarLog, nullptr, tarLogThread, nullptr);

    return a.exec();
}
