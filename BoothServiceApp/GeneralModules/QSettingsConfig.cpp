#include "QSettingsConfig.h"
#include <QDebug>
#include <QTextCodec>
#include <QTime>

QSettingsConfig *QSettingsConfig::self = nullptr;

QSettingsConfig *QSettingsConfig::instance()
{
    if (self == nullptr) {
        self = new QSettingsConfig();
    }
    return self;
}

QSettingsConfig::QSettingsConfig(QObject *parent) : QObject(parent)
{
    m_strProvinceName = "";
    m_nStartMsec = 0;
    m_nEndMsec = 0;

//    //测试模式 0-生产 1-测试
//    getConfigValue("ServerInfo", "TestMode", 1);
//    getConfigValue("ReadMe", "TestMode", "测试模式 0-生产 1-测试");

//    //本程序绝对路径(以'/'结尾)
//    getConfigValue("ServerInfo", "AppPath", "/home/apps/TwBoothService/");
//    getConfigValue("ReadMe", "AppPath", "本程序绝对路径(以'/'结尾)");

//    //车道号
//    getConfigValue("ServerInfo", "LaneId", "43017B0278");
//    getConfigValue("ReadMe", "LaneId", "车道号");

//    //外场服务WS地址
//    getConfigValue("ServerInfo", "ServerURL", "ws://192.168.13.62:10090/edge-modules-booth/booth/ws?appId=43017B0278");
//    getConfigValue("ReadMe", "ServerURL", "外场服务WS地址");

//    //是否启动收费键盘 0-关闭 1-开启
//    getConfigValue("KeyBoard", "KeyBoardOpen", 0);
//    getConfigValue("ReadMe", "KeyBoardOpen", "是否启动收费键盘 0-关闭 1-开启");

//    //收费键盘串口号
//    getConfigValue("KeyBoard", "SerialPortKeyBoard", "ttyUSB0");
//    getConfigValue("ReadMe", "SerialPortKeyBoard", "收费键盘串口号");

//    //收费键盘波特率
//    getConfigValue("KeyBoard", "BaudrateKeyBoard", 9600);
//    getConfigValue("ReadMe", "BaudrateKeyBoard", "收费键盘波特率");

//    //是否启动打印机 0-关闭 1-开启
//    getConfigValue("Printer", "PrinterOpen", 0);
//    getConfigValue("ReadMe", "PrinterOpen", "是否启动打印机 0-关闭 1-开启");

//    //发票打印机串口号
//    getConfigValue("Printer", "SerialPortPrinter", "ttyUSB0");
//    getConfigValue("ReadMe", "SerialPortPrinter", "发票打印机串口号");

//    //发票打印机波特率
//    getConfigValue("Printer", "BaudratePrinter", 9600);
//    getConfigValue("ReadMe", "BaudratePrinter", "发票打印机波特率");

//    //是否启动读卡器 0-关闭 1-开启
//    getConfigValue("CardReader", "CardReaderOpen", 0);
//    getConfigValue("ReadMe", "CardReaderOpen", "是否启动读卡器 0-关闭 1-开启");

//    //读卡器串口号
//    getConfigValue("CardReader", "CardReaderCom", "/dev/ttyUSB0");
//    getConfigValue("ReadMe", "CardReaderCom", "读卡器串口号");

//    //PSAM卡槽号
//    getConfigValue("CardReader", "PsamSockId", 1);
//    getConfigValue("ReadMe", "PsamSockId", "PSAM卡槽号");

//    //读卡器类型 0-特微 1-汉鼎 2-航天
//    getConfigValue("CardReader", "CardReaderType", 0);
//    getConfigValue("ReadMe", "CardReaderType", "读卡器类型 0-特微 1-汉鼎 2-航天");
}

QSettingsConfig::~QSettingsConfig()
{
}

QString QSettingsConfig::getConfigValue(QString strGroup, QString strKey, QString strDefaultValue)
{
    m_mutex.tryLock(500);

    QSettings settings(INI_PATH, QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    settings.beginGroup(strGroup);
    if (settings.value(strKey).isNull()) {
        qDebug() << strGroup << "-" << strKey << "不存在，创建默认值";
        settings.setValue(strKey, strDefaultValue);
        settings.sync();
    }
    QVariant var = settings.value(strKey);
    settings.endGroup();

    m_mutex.unlock();

    return var.toString();
}

int QSettingsConfig::getConfigValue(QString strGroup, QString strKey, int nDefaultValue)
{
    m_mutex.tryLock(500);

    QSettings settings(INI_PATH, QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    settings.beginGroup(strGroup);
    if (settings.value(strKey).isNull()) {
        qDebug() << strGroup << "-" << strKey << "不存在，创建默认值";
        settings.setValue(strKey, nDefaultValue);
        settings.sync();
    }
    QVariant var = settings.value(strKey);
    settings.endGroup();

    m_mutex.unlock();

    return var.toInt();
}

double QSettingsConfig::getConfigValue(QString strGroup, QString strKey, double fDefaultValue)
{
    m_mutex.tryLock(500);

    QSettings settings(INI_PATH, QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    settings.beginGroup(strGroup);
    if (settings.value(strKey).isNull()) {
        qDebug() << strGroup << "-" << strKey << "不存在，创建默认值";
        settings.setValue(strKey, fDefaultValue);
        settings.sync();
    }
    QVariant var = settings.value(strKey);
    settings.endGroup();

    m_mutex.unlock();

    return var.toDouble();
}

void QSettingsConfig::setConfigValue(QString strGroup, QString strKey, QString strValue)
{
    m_mutex.tryLock(500);

    QSettings settings(INI_PATH, QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    settings.beginGroup(strGroup);
    settings.setValue(strKey, strValue);
    settings.sync();
    settings.endGroup();

    m_mutex.unlock();

    return;
}

void QSettingsConfig::setConfigValue(QString strGroup, QString strKey, int nValue)
{
    m_mutex.tryLock(500);

    QSettings settings(INI_PATH, QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    settings.beginGroup(strGroup);
    settings.setValue(strKey, nValue);
    settings.sync();
    settings.endGroup();

    m_mutex.unlock();

    return;
}

void QSettingsConfig::setConfigValue(QString strGroup, QString strKey, double fValue)
{
    m_mutex.tryLock(500);

    QSettings settings(INI_PATH, QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));

    settings.beginGroup(strGroup);
    settings.setValue(strKey, fValue);
    settings.sync();
    settings.endGroup();

    m_mutex.unlock();

    return;
}

QString QSettingsConfig::getProvinceNameFromConfig()
{
    if (m_strProvinceName.isEmpty()) {
        switch (SETTINGS("ServerInfo", "LaneId", "43017B0278").leftRef(2).toInt()) {
        case 12: m_strProvinceName = "天津"; break;
        case 41: m_strProvinceName = "河南"; break;
        case 43: m_strProvinceName = "湖南"; break;
        case 62: m_strProvinceName = "甘肃"; break;
        default: m_strProvinceName = "甘肃"; break;
        }
        qDebug() << "LaneId:" << SETTINGS("ServerInfo", "LaneId", "43017B0278") << "，对应省份: " << m_strProvinceName;
    }
    return m_strProvinceName;
}
