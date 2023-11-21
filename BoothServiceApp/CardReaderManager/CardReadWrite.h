#ifndef CARDREADWRITE_H
#define CARDREADWRITE_H

#include <QObject>
#include "json/json.h"

#define BLOCK_SIZE 32
#define BLOCK_CNT_PER_SEC 4

//M1卡(cardtype==6)密钥，KeyA仅可用于读卡，KeyB可用于读写
#define M1_READ_CARD_KEY_TYPE 0
#define M1_WRITE_CARD_KEY_TYPE 1
#define M1_CARD_KEY_A "A0A1A2A3A4A5"
#define M1_CARD_KEY_B_SEC_FIRST "3E7935487970"  //0扇区密钥
#define M1_CARD_KEY_B_SEC_OTHER "22782E443F43"  //1~31扇区密钥
#define M1_CARD_KEY_SEC_TAIL    "FFFFFFFFFFFF"  //32扇区读写密钥（32扇区单独占4个扇区的空间，共128~143十六个block）

#define M1_CARD_SECTOR_NUM 36
#define M1_CARD_BLOCK_NUM BLOCK_CNT_PER_SEC * M1_CARD_SECTOR_NUM

//格式块卡读写密钥均使用FFFFFFFF即可
#define BLOCK_CARD_KEY_TYPE 0
#define BLOCK_CARD_SEC_0_KEY "FFFFFFFFFFFF"
#define BLOCK_CARD_SEC_1_KEY "583524386E39"

#define BLOCK_CARD_SECTOR_NUM 2
#define BLOCK_CARD_BLOCK_NUM BLOCK_CNT_PER_SEC * BLOCK_CARD_SECTOR_NUM

//动态库接口声明
typedef int (*TYPE_JT_OpenReader  )(int nMode, char *sParas);
typedef int (*TYPE_JT_CloseReader )(int nHandle);
typedef int (*TYPE_JT_OpenCard    )(int nHandle, int* pCardPlace,char* sPhysicsCardno);
typedef int (*TYPE_JT_CloseCard   )(int nHandle);
typedef int (*TYPE_JT_LEDDisplay  )(int nHandle, int nRed,  int nGreen,int nBlue);
typedef int (*TYPE_JT_AudioControl)(int nHandle, int nTimes, int nVoice );
typedef int (*TYPE_JT_CPUCommand  )(int nHandle, char* sCommand, int nLenCom, char* sReply, int* pLenRep);
typedef int (*TYPE_JT_SamReset    )(int nHandle, int nSockID, int nProtocolType );
typedef int (*TYPE_JT_SamCommand  )(int nHandle, int nSockID, char* sCommand, int nLenCom, char* sReply,  int* pLenRep);
typedef int (*TYPE_JT_GetStatus   )(int nHandle, int* pStatusCode);
typedef int (*TYPE_JT_GetStatusMsg)(int nStatusCode, char* sStatusMsg, int nStatusMsgLen);
typedef int (*TYPE_JT_ReaderVersion)(int nHandle,char* sRDVersion,int nRDVerMaxLen,char* sAPIVersion,int nAPIVerMaxLen);
typedef int (*TYPE_JT_LoadKey     )(int nHandle, int nKeyType, int nSec, char* sKeyValue);
typedef int (*TYPE_JT_ReadBlock   )(int nHandle, int nKeyType, int nBlock, char* sReply);
typedef int (*TYPE_JT_WriteBlock  )(int nHandle, int nKeyType, int nBlock, char* sData);

using namespace std;

class QLibrary;
class QTimer;
class CardReadWrite : public QObject
{
    Q_OBJECT

public:
    static CardReadWrite *getInstance();
    virtual ~CardReadWrite();

public:
    bool initPsam();
    void openCard();
    void closeCard();
    bool openReader();
    void sendOpenReaderResult();

    void readCpcCard();
    void writeCpcCard(string strJson);

    void readCpuCard();
    void writeCpuCard(string strJson);

    void readM1Card();
    void writeM1Card(string strJson);

    void readBlockCard();

    void sendPsamAuthPost();
    void recvPsamAuthPost(string strJson);

    int getCardType() { return m_nCardType; }

public:
    //cpc
    bool cpcSelectFile(QString strDir, int nFileNo);
    bool cpcReadFileContent(int nOffSet, int nLen, QString &strContent);
    void sendReadCpcCardResult(int nErrCode, QString MFEF01 = "", QString MFEF02 = "", QString DFEF01 = "", QString DFEF02 = "", QString DFEF04 = "");
    void sendWriteCpcCardResult(int nErrCode, string strMessage);
    bool writeCpcData(int nFileNo, QString strContent);

    //cpu
    void sendReadCpuCardResult(int nErrCode, QString DF0002 = "", QString DF0015 = "", QString DF0019 = "");
    void sendWriteCpuCardResult(int nErrCode, string strMessage, int Balance = -1, QString MAC2 = "", QString TAC = "", QString termId = "", QString termSerialNo = "", QString tradeSerialNo = "");
    bool openCpuCardDF01();
    bool readCpuCard0015();
    bool readCpuCard0019();
    bool readCpuCard0002();
    bool cpuCardConsume(unsigned int nConsumeMoney, QString strTradeTime, QString strContent);

    //m1
    bool writeM1CardBlock(int nBlockNo);  //将M1卡信息缓存区中指定的数据块写入卡片相应位置
    bool writeM1CardBlock(int nBlockNo, string strData);  //将字符串写入卡片指定数据块
    void printM1CardInfo(int nBlockNo);  //打印M1卡信息缓存区中指定的数据块
    void sendReadM1CardResult(int nErrCode, string strMessage);
    void sendWriteM1CardResult(int nErrCode, string strMessage);

    //block
    void printBlockCardInfo(int nBlockNo);  //打印M1卡信息缓存区中指定的数据块
    void sendReadBlockCardResult(int nErrCode, string strMessage, string userId = "");

private slots:
    void slotTimerPsamAuth();
    void slotTimerReaderState();

private:
    explicit CardReadWrite(QObject *parent = nullptr);
    static CardReadWrite *self;

    bool loadLibrary();
    void getRandom();
    void psamAuth(QString authInfo);
    bool openPsamDF01();

private:
    TYPE_JT_OpenReader    JT_OpenReader   ;
    TYPE_JT_CloseReader   JT_CloseReader  ;
    TYPE_JT_OpenCard      JT_OpenCard     ;
    TYPE_JT_CloseCard     JT_CloseCard    ;
    TYPE_JT_LEDDisplay    JT_LEDDisplay   ;
    TYPE_JT_AudioControl  JT_AudioControl ;
    TYPE_JT_CPUCommand    JT_CPUCommand   ;
    TYPE_JT_SamReset      JT_SamReset     ;
    TYPE_JT_SamCommand    JT_SamCommand   ;
    TYPE_JT_GetStatus     JT_GetStatus    ;
    TYPE_JT_GetStatusMsg  JT_GetStatusMsg ;
    TYPE_JT_ReaderVersion JT_ReaderVersion;
    TYPE_JT_LoadKey       JT_LoadKey      ;
    TYPE_JT_ReadBlock     JT_ReadBlock    ;
    TYPE_JT_WriteBlock    JT_WriteBlock   ;

    QLibrary *m_pLibReader;
    QTimer *m_pTimerPsamAuth;
    QTimer *m_pTimerReaderState;

    int m_nPsamSockId;
    int m_nHandle;
    int m_nCardType;
    int m_nPostAuthCnt;

    bool m_bHasPsamAuthed;
    bool m_bTestMode;

    std::string m_strPhysicsCardno;  //物理卡号
    QString m_strPsamSerialNo;//序列号
    QString m_strPsamVersion;//PSAM版本号
    QString m_strKeyCardType;//秘钥卡类型
    QString m_strTermNo;//终端号
    QString m_strRandom;

    //cpc
    QString m_strAreaCode;//区域代码
    QString m_strCPCID ;//CPCID

    //cpu
    QString m_strFile0015;
    QString m_strFile0019;
    QString m_strFile0002;
    QString m_strCardIssuer;  //0015卡片发行方标识
    QString m_strCardNo;  //0015卡片编码

    //m1
    char m_strM1CardInfo[M1_CARD_SECTOR_NUM][BLOCK_CNT_PER_SEC][BLOCK_SIZE];  //M1卡读取信息缓存[扇区号][块号][字符]
    Json::Value m_jsonCartList;

    //block
    char m_strBlockCardInfo[BLOCK_CARD_SECTOR_NUM][BLOCK_CNT_PER_SEC][BLOCK_SIZE];  //块卡读取信息缓存[扇区号][块号][字符]
};

#endif // CardReaderCom_H
