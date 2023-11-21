#include "clogfile.h"

void RecordLogFile(QString strLog)
{

    static QMutex mutex;
    mutex.lock();

    QString msg = strLog;

    QDateTime dateTime = QDateTime::currentDateTime();
    QString time = dateTime.toString("yyyyMMdd");
    QString t_StrDir = "/opt/log";
    QString fileName =   QObject::tr("%1/LogBoothService%2.log").arg(t_StrDir, time);
    QFileInfo info(fileName);
    if(!info.exists())  //不存在就创建一个，对应1天1个日志
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadWrite|QIODevice::Append))
            qDebug() << file.errorString();
        QTextStream Stream(&file);
        QString str =  dateTime.toString("yyyyMMdd") + ":    岗亭服务日志" + "\r\n\r\n\r\n";
        Stream << str;
        file.close();
    }
    qint64 fileSize = info.size();
    if(fileSize > IFLOG_MAX_FILE_SIZE)    //超过指定大小删除当前日志,再创建1个
    {
        QFile::remove(fileName);

        QFile file(fileName);
        if (!file.open(QIODevice::ReadWrite|QIODevice::Append))
            qDebug() << file.errorString();
        QTextStream Stream(&file);
        Stream << "今天最开始的1份超过指定大小，已删除\n";
        QString str =  dateTime.toString("yyyyMMdd") + ":    岗亭服务日志" + "\r\n\r\n\r\n";
        Stream << str;
        file.close();
    }
    QString totalTime = dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString allStr;
    allStr = QObject::tr("[%1]: %2").arg(totalTime, msg) + "\n";
    QFile file(fileName);
    if (!file.open(QIODevice::ReadWrite|QIODevice::Append))
        qDebug() << file.errorString();
    QTextStream Stream(&file);
    Stream << allStr;
    file.close();
    mutex.unlock();
}
