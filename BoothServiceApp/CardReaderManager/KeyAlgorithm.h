//---------------------------------------------------------------------------

#ifndef KeyAlgorithmH
#define KeyAlgorithmH
//---------------------------------------------------------------------------
#include <string>
#include <QByteArray>
using namespace std;

#include "d3des.h"       //DES算法实现
#include "sm4.h"         //SM4算法实现

//#define USER_SM2

#ifdef USER_SM2
#include "mysm2.h"       //SM2算法实现
#endif

template<typename T1,typename T2>
bool HexToBin(const T1* sStr,T2* sDist,int nLen)
{
	QByteArray baStr = QByteArray((const char*)sStr,nLen*2);
	QByteArray baDist = QByteArray::fromHex(baStr);
	memcpy((char*)sDist,baDist.data(),nLen);
	return true;
}

template<typename T1,typename T2>
static bool BinToHex(const T1* sStr,T2* sDist,int nLen)
{
	QByteArray baStr = QByteArray((const char*)sStr,nLen);
	QByteArray baDist = baStr.toHex();
	memcpy((char*)sDist,baDist.data(),nLen*2);
	return true;
}

//算法类实现：DES、3DES、SM4、SM2、SM3

class TKeyAlgorithm
{
public:
    struct _arr_man
    {
        unsigned char* _p;
        _arr_man(unsigned char* p):_p(p){}
        ~_arr_man(){if(_p)delete[]_p;}
    };
//===================================================================================================================================
//===================================================================================================================================
//===================================================================================================================================
//  以下为DES/3DES算法

/*
  功能：使用DES或3DES进行数据加密 （ 64位分组，密钥不分散）

  参数：data     —— （in）明文数据; (Hex格式) (注意：内部会补齐数据,补齐格式：明文长度+明文+填充数据）
        key      —— （in）密钥；    (Hex格式)
        IsTriple —— （in）true:3DES、false:DES ，默认3DES

  返回值：（明文长度+明文数据+填充数据）的密文  (Hex格式)

  备注：填充数据的补齐规则参考另一个重载bin版本的“加密数据自动补齐原则”
*/
static QByteArray  EncryptData(const QByteArray plaintext, const QByteArray key, bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
      参数：rvalue   —— （out） （明文长度+明文+填充数据）的密文（Bin格式）；
            rlen     —— （out） 密文长度（字节）
            bin      —— （in）  密明文数据 （Bin格式） （注意：内部会自动补齐0x80 00 ...)
            len      —— （in）  明文长度（字节）
            key      —— （in）  密钥；     （Bin格式）
            IsTriple —— （in）  true:3DES、false:DES ，默认3DES

      返回值：无

      数据自动补齐原则：
           1.data数据分解成8字节数据块，标号为D1，D2，D3，D4等等。最后一个数据块的长度可能不足8字节。
           2.如果最后的数据块长度不足8字节，则在其后加上16进制数字'80'，如果达到8字节长度，则转入下一步；否则在其后加
             入16进制数字'00 '直到长度达到8字节。

*/
static void  EncryptData(unsigned char* rvalue, int& rlen, const unsigned char* bin, int len, const unsigned char key[16], bool IsTriple=true);

//===================================================================================================================================
//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES进行数据加密
    参数：  data    ——（in）数据明文（Hex格式） ，长度必须为8字节倍数
            key     ——（in）密钥    （Hex格式）
            IsTriple —-（in）true:3DES、false:DES ，默认3DES

    返回值：data数据加密后的密文  (Hex格式)
*/
static QByteArray  EncryptWithDes(QByteArray data, QByteArray key, bool IsTriple=true);
//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES加密8字节数据块
    参数：  rvalue   —— （out）8字节密文数据    （Bin格式）
            data     —— （int）8字节明文数据    （Bin格式）
            key      —— （in）密钥；            （Bin格式）
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

    返回值：无
*/
static void  EncryptWithDes(unsigned char rvalue[8],const unsigned char data[8],const unsigned char key[16], bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES加密数据（ECB模式）
    参数：  rvalue   —— （out）密文数据（Bin格式）
            rlen     —— （out）密文长度（字节）
            data     —— （in） 明文数据（Bin格式） ,长度必须为8字节倍数
            dlen     —— （in） 明文长度（字节）
            key      —— （in） 密钥；  （Bin格式）
            IsTriple —— （in）true:3DES、false:DES ，默认3DES

    返回值：无

    ECB模式：
         1.第一组数据D1进行DES加密,得到第一组密文C1
         2.第二组数据D2进行DES加密,得到第二组密文C2，之后的数据以此类推，得到Cn
         3.按顺序连为C1C2C3......Cn即为加密结果。
*/
static void  EncryptWithECB(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen,const unsigned char key[16], bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES加密数据（CBC模式）
    参数：  rvalue   —— （out）密文数据（Bin格式）
            rlen     —— （out）密文长度（字节）
            data     —— （in） 明文数据（Bin格式） ,长度必须为8字节倍数
            dlen     —— （in） 明文长度（字节）
            atc      —— （in） 8字节的初始值 （Bin格式）
            key      —— （in） 密钥；  （Bin格式）
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

    返回值：无

    CBC模式：
         1.第一组数据D1与初始化向量I异或后的结果进行DES加密得到第一组密文C1（初始化向量I为全零）
         2.第二组数据D2与第一组的加密结果C1异或以后的结果进行DES加密，得到第二组密文C2，之后的数据以此类推，得到Cn
         3.按顺序连为C1C2C3......Cn即为加密结果。
*/
static void  EncryptWithCBC(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen, const unsigned char atc[8],const unsigned char key[16], bool IsTriple=true);

//===================================================================================================================================
/*
    函数功能：使用DES或3DES进行数据解密
    参数：  data    —— （in）密文数据（Hex格式）（注意，内部会自动填充0x00 .. ）
            key     —— （in）密钥    （Hex格式）
            IsTriple ——（in）true:3DES、false:DES ，默认3DES

    返回值：明文数据 (Hex格式)  （注意，最后的明文数据包括了补齐数据）

*/
static QByteArray DecryptData(const QByteArray data, const QByteArray key, bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
      参数：rvalue   —— （out）明文数据（含填充数据） （Bin格式）；
            rlen     —— （out）明文长度（字节）
            bin      —— （in） 密文数据（Bin格式）  （注意，内部会自动填充0x00 .. ）
            len      —— （in） 密文长度（字节）
            key      —— （in） 密钥；  （Bin格式）
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

      返回值：无
*/
static void DecryptData(unsigned char* rvalue, int& rlen, const unsigned char* bin, int len,const unsigned char key[16], bool IsTriple=true);

//===================================================================================================================================
/*
   函数功能：使用DES或3DES进行数据解密 （ 64位（8字节）分组，密钥不分散）


    函数功能：使用DES或3DES进行数据加密
    参数：  data     ——（in）密文（Hex格式）,长度必须为8字节倍数
            key      ——（in）密钥 （Hex格式）
            IsTriple ——（in）true:3DES、false:DES ，默认3DES

    返回值：data数据加密后的密文  (Hex格式)
*/
static QByteArray  DecryptWithDes(QByteArray data, QByteArray key, bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES解密8字节数据块
    参数：  rvalue   —— （out）8字节明文数据  （Bin格式）
            data     —— （in）8字节密文数据   （Bin格式）
            key      —— （in）密钥；          （Bin格式）
            IsTriple —— （in）true:3DES、false:DES ，默认3DES

    返回值：无
*/
static void  DecryptWithDes(unsigned char rvalue[8],const unsigned char data[8],const unsigned char key[16], bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES解密数据（ECB模式）
    参数：  rvalue   —— （out）明文数据（Bin格式）
            rlen     —— （out）明文长度（字节）
            data     —— （in） 密文数据（Bin格式） ,长度为8字节倍数
            dlen     —— （in） 密文长度（字节）
            key      —— （in） 密钥；  （Bin格式）
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

    返回值：无

    ECB模式：
         1.首先将数据按照8个字节一组进行分组得到C1C2C3......Cn；
         1.第一组数据C1进行DES解密,得到第一组明文D1
         2.第二组数据C2进行DES解密,得到第二组明文D2，之后的数据以此类推，得到Dn
         3.按顺序连为D1D2D3......Dn即为解密结果。
*/
static void  DecryptWithECB(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen,const unsigned char key[16], bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
    函数功能：使用DES或3DES解密数据（CBC模式）
    参数：  rvalue   —— （out）明文数据      （Bin格式）
            rlen     —— （out）明文长度（字节）
            data     —— （in） 密文数据      （Bin格式）
            dlen     —— （in） 密文长度（字节）      长度为8字节倍数
            atc      —— （in） 8字节的初始值 （Bin格式）
            key      —— （in） 密钥；        （Bin格式）
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

    返回值：无

    CBC模式：
            1)	首先将数据按照8个字节一组进行分组得到C1C2C3......Cn；
            2)	将第一组数据进行解密后与初始化向量I进行异或得到第一组明文D1 （注意：一定是先解密再异或）；
            3)	将第二组数据C2进行解密后与第一组密文数据进行异或得到第二组数据D2
            4)	之后依此类推，得到Dn
            5)	按顺序连为D1D2D3......Dn即为解密结果。
*/
static void  DecryptWithCBC(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen, const unsigned char atc[8],const unsigned char key[16], bool IsTriple=true);

//===================================================================================================================================
/*  函数功能： 使用DES算法计算MAC

    备注：     函数会自行补上填充数据

    填充规则：1.如果最后的数据块长度是8字节的话，则在其后加上8字节的16进制数字'80 00 00 00 00 00 00 00 '，
              2.如果最后的数据块长度不足8字节，则在其后加上16进制数字'80'，如果达到8字节长度，则转入下一步；
                否则在其后加入16进制数字'00 '直到长度达到8字节。

    Hex方式：
         参数：  data     ——（in） 计算MAC码数据    (Hex格式）
                 random   ——（in） 8字节随机数（初始值）      (Hex格式）
                 key      ——（in） 密钥             (Hex格式）
                 IsTriple ——（in） true:3DES、false:DES ，默认3DES

        返回值：MAC码
*/
static QByteArray CalcMac(const QByteArray data, const QByteArray random, const QByteArray key, bool IsTriple=true);

//-----------------------------------------------------------------------------
/*
    备注：     函数会自行补上填充数据

    填充规则：1.如果最后的数据块长度是8字节的话，则在其后加上8字节的16进制数字'80 00 00 00 00 00 00 00 '，
              2.如果最后的数据块长度不足8字节，则在其后加上16进制数字'80'，如果达到8字节长度，则转入下一步；
                否则在其后加入16进制数字'00 '直到长度达到8字节。

     参数：  mac      ——（out）4字节MAC码      （Bin格式）
             bin      ——（in） 计算MAC数据     （Bin格式）
             len      ——（in） bin的长度（字节）
             atc      ——（in） 8字节随机数（初始值）（Bin格式）
             key      ——（in） 密钥            （Bin格式）
             IsTriple ——（in） true:3DES、false:DES ，默认3DES

    返回值：无
*/
static void CalcMac(unsigned char mac[4], const unsigned char* bin, int len, const unsigned char atc[8], const unsigned char key[16], bool IsTriple=true);


//-----------------------------------------------------------------------------
/*
    函数功能：计算交易MAC （MAC数据内部填充）

         参数：  data     ——（in） 计算MAC码数据         (Hex格式）
                 random   ——（in） 8字节随机数（初始值） (Hex格式）
                 sessionK —— (in) 8字节过程密钥因子  （Hex）
                 key      ——（in） 密钥             (Hex格式）
                iAlg     ——（in） 算法：0-3des，4-sm4

        返回值：MAC码
*/
static QByteArray CalcTradeMac(const QByteArray data, const QByteArray random, const QByteArray sessionKeySeed,const QByteArray key,int iAlg);

//-----------------------------------------------------------------------------
/*
    函数功能：计算交易TAC （TAC数据内部填充）

         参数：  data     ——（in） 计算TAC码数据         (Hex格式）
                 random   ——（in） 8字节随机数（初始值） (Hex格式）
                 key      ——（in） 密钥             (Hex格式）
                iAlg      ——（in） 算法：0-3des，4-sm4

        返回值：TAC码
*/
static QByteArray CalcTradeTac(const QByteArray data, const QByteArray random,const QByteArray key,int iAlg);




//==================================================================================================================================

/* 子密钥推导

     推导算法：参考相关文档说明

     参数：   data   ——（in） 分散系数  （长度为8字节倍数） （Hex格式）
              key    ——（in） 密钥 （只能是双倍长密钥）     （Hex格式）
              divCnt ——（in） 分散次数 (默认1）  data数据的字节长度/8

     返回值：分散后的子密钥

     备注：分散系数从左往右依次使用
*/
static QByteArray  DiversifyKey(QByteArray data, QByteArray key,int divCnt=1 );

//-----------------------------------------------------------------------------
/* 子密钥推导 

   推导算法：参考相关文档说明

   参数：   data     ——（in）8字节分散系数     （Hex格式）
            key      ——（in）密钥              （Hex格式）
            IsTriple ——（in）true:3DES、false:DES ，默认3DES

   返回值：分散后的子密钥

   备注：该函数只支持一次分散，如果需要进行多次分散需要多次调用该函数
*/
static QByteArray  GetSubkey(QByteArray data, QByteArray key,bool IsTriple=true);

//-----------------------------------------------------------------------------
 /* 子密钥推导(非高速应用）
    推导算法：参考源码
*/
static void  GetSubkey(unsigned char* rvalue, int& rlen, unsigned char* data, int dlen, unsigned char* key,bool IsTriple=true);

//-----------------------------------------------------------------------------
 /* 子密钥推导(非高速应用）
    推导算法：参考源码
*/
static QByteArray  GenDiversifiedKey(QByteArray sData, QByteArray sKey, int iDiversifiedLevel);

//==================================================================================================================================

// 数据取反

//Hex
static QByteArray  Invert(QByteArray src);
//bin
static void  Invert(char *des, char *src, int length);
static void  Invert(unsigned char *des, unsigned char *src, int length);

//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//以下为国密算法 SM4

/*
Hex版本
    函数功能：使用SM4算法进行数据加密（128位分组，密钥不分散）

    参数：  plaintext —— （in） 明文数据   （注意：内部会进行明文拼装）
            key       —— （out）16字节密钥

    返回值：数据密文（格式：明文数据长度 + 明文数据 + 填充数据）

    补齐规则：  1.在明文数据前面加上明文数据长度Ld，组成新的数据；
                2.按16字节分块，最后一块如果不满16字节，需要补齐 0x80 00 ...
*/
static QByteArray  EncryptData_SM4(const QByteArray plaintext, const QByteArray key);

//-----------------------------------------------------------------------------
/*
Bin版本
    函数功能：使用SM4算法进行数据加密（128位分组，密钥不分散）

    参数：  rvalue    —— （out）密文数据（格式：明文数据 + 填充数据）
            rlen      —— （out）密文长度（注意
            plaintext —— （in） 明文数据（注意：内部会进行明文拼装）
            key       —— （in） 16字节密钥

    返回值：

    补齐规则：按16字节分块，最后一块如果不满16字节，需要补齐 0x80 00 ...
*/
static void  EncryptData_SM4(unsigned char* rvalue, int& rlen, const unsigned char* plaintext, int textlen, const unsigned char key[16]);

//===================================================================================================================================
/*
Hex版本
    函数功能：使用SM4算法进行数据加密
    参数：  plaintext ——（in）明文数据（Hex格式），长度为16字节倍数
            key       ——（in）密钥

    返回值：密文  (Hex格式)
*/
static QByteArray  EncryptWithSM4(QByteArray plaintext, QByteArray key);

//-----------------------------------------------------------------------------
/*
Bin版本
    函数功能：使用SM4算法加密16字节数据块
    参数：  rvalue    —— （out）16字节密文数据
            plaintext —— （in） 16字节明文数据
            key       —— （in） 密钥；

    返回值：无
*/
static void  EncryptWithSM4(unsigned char rvalue[16],const unsigned char plaintext[16],const unsigned char key[16]);

//-----------------------------------------------------------------------------
/*
Bin版本
    函数功能：使用SM4算法加密数据（ECB模式）
    参数：  rvalue   —— （out）密文数据（Bin格式）
            rlen     —— （out）密文长度（字节）
            data     —— （in） 明文数据（Bin格式）  ,长度为16字节倍数
            dlen     —— （in） 明文长度（字节）
            key      —— （in） 密钥；

    返回值：无

    ECB模式：
         1.第一组数据D1进行SM4加密,得到第一组密文C1
         2.第二组数据D2进行SM4加密,得到第二组密文C2，之后的数据以此类推，得到Cn
         3.按顺序连为C1C2C3......Cn即为加密结果。
*/
static void  EncryptWithECB_SM4(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen,const unsigned char key[16]);

//-----------------------------------------------------------------------------
/*

Bin版本
    函数功能：使用SM4算法加密数据（CBC模式）
    参数：  rvalue   —— （out）密文数据（Bin格式）
            rlen     —— （out）密文长度（字节）
            data     —— （in） 明文数据（Bin格式） ,长度为16字节倍数
            dlen     —— （in） 明文长度（字节）
            atc      —— （in） 16字节的初始值
            key      —— （in） 密钥；

    返回值：无

    CBC模式：
         1.第一组数据D1与初始化向量I异或后的结果进行SM4加密得到第一组密文C1（初始化向量I为全零）
         2.第二组数据D2与第一组的加密结果C1异或以后的结果进行SM4加密，得到第二组密文C2，之后的数据以此类推，得到Cn
         3.按顺序连为C1C2C3......Cn即为加密结果。
*/
static void  EncryptWithCBC_SM4(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen, const unsigned char atc[16],const unsigned char key[16]);


//mode: 加密模式，0 - ECB，其他-CBC
//atc: 初始向量，mode 非0 时有效
static QByteArray   EncryptWithSM4(QByteArray plaintext, QByteArray key,int mode,QByteArray atc="");

//===================================================================================================================================
/*
Hex版本
    函数功能：使用SM4算法进行数据解密
    参数：  data    —— （in）密文数据（Hex格式）（注意，内部会自动填充0x00 .. ）
            key     —— （in）密钥

    返回值：明文数据 (Hex格式)  （注意，最后的明文数据包括了补齐数据）

*/
static QByteArray DecryptData_SM4(const QByteArray data, const QByteArray key);

//-----------------------------------------------------------------------------
/* Bin版本
       函数功能：使用SM4算法数据解密
      参数：rvalue     —— （out）明文数据（含填充数据） （Bin格式）；
            rlen       —— （out）明文长度（字节）
            ciphertext —— （in） 密文数据（Bin格式）  （注意，内部会自动填充0x00 .. ）
            textlen    —— （in） 密文长度（字节）
            key        —— （in） 密钥；

      返回值：无
*/
static void DecryptData_SM4(unsigned char* rvalue, int& rlen, const unsigned char* ciphertext, int textlen,const unsigned char key[16]);

//===================================================================================================================================
/*
Hex版本

   函数功能：使用SM4进行数据解密 （ 64位（8字节）分组，密钥不分散）

    参数：  data     ——（in）密文（Hex格式） ,长度为16字节倍数
            key      ——（in）密钥

    返回值：明文  (Hex格式)
*/
static QByteArray  DecryptWithSM4(QByteArray data, QByteArray key);

//-----------------------------------------------------------------------------
/*
Bin版本
    函数功能：使用SM4进行数据解密
    参数：  rvalue   —— （in）16字节明文数据
            data     —— （in）16字节密文数据
            key      —— （in）密钥；

    返回值：无
*/
static void  DecryptWithSM4(unsigned char rvalue[16],const unsigned char data[16],const unsigned char key[16]);

//-----------------------------------------------------------------------------
/*
Bin版本
    函数功能：使用SM4进行数据解密（ECB模式）
    参数：  rvalue   —— （out）明文数据（Bin格式）
            rlen     —— （out）明文长度（字节）
            data     —— （in） 密文数据（Bin格式）
            dlen     —— （in） 密文长度（字节）
            key      —— （in） 密钥；
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

    返回值：无

    ECB模式：
         1.首先将数据按照16个字节一组进行分组得到C1C2C3......Cn；
         1.第一组数据C1进行SM4解密,得到第一组明文D1
         2.第二组数据C2进行SM4解密,得到第二组明文D2，之后的数据以此类推，得到Dn
         3.按顺序连为D1D2D3......Dn即为解密结果。
*/
static void  DecryptWithECB_SM4(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen,const unsigned char key[16]);

//-----------------------------------------------------------------------------
/*
Bin版本

    函数功能：使用SM4进行数据解密（CBC模式）
    参数：  rvalue   —— （out）明文数据（Bin格式）
            rlen     —— （out）明文长度（字节）
            data     —— （in） 密文数据（Bin格式）
            dlen     —— （in） 密文长度（字节）
            atc      —— （in） 16字节的初始值
            key      —— （in） 密钥；
            IsTriple —— （in） true:3DES、false:DES ，默认3DES

    返回值：无

    CBC模式：
            1)	首先将数据按照16个字节一组进行分组得到C1C2C3......Cn；
            2)	将第一组数据进行解密后与初始化向量I进行异或得到第一组明文D1 （注意：一定是先解密再异或）；
            3)	将第二组数据C2进行解密后与第一组密文数据进行异或得到第二组数据D2
            4)	之后依此类推，得到Dn
            5)	按顺序连为D1D2D3......Dn即为解密结果。
*/
static void  DecryptWithCBC_SM4(unsigned char* rvalue,unsigned& rlen,const unsigned char* data,const unsigned dlen, const unsigned char atc[16],const unsigned char key[16]);

//===================================================================================================================================
/*
Hex版本

    函数功能： 使用SM4计算MAC

    备注：     函数会自行补上填充数据

    填充规则：1.如果最后的数据块长度是16字节的话，则在其后加上16字节的16进制数字'80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00'，
              2.如果最后的数据块长度不足16字节，则在其后加上16进制数字'80'，如果达到16字节长度，则转入下一步；
                否则在其后加入16进制数字'00 '直到长度达到16字节。
                

    参数：  data     ——（in） 计算MAC码数据
            random   ——（in） 16字节随机数（初始值）（不16字节补0x00）
            key      ——（in） 密钥

    返回值：MAC码
*/
static QByteArray CalcMac_SM4(const QByteArray data, const QByteArray random, const QByteArray key);

//==================================================================================================================================
/* 
  Bin方式：
     参数：  mac      ——（out）4字节MAC码
             bin      ——（in） 计算MAC数据
             len      ——（in） bin的长度（字节）
             atc      ——（in） 16字节随机数（初始值）（注意：不足16字节补0）
             key      ——（in） 密钥
             IsTriple ——（in） true:3DES、false:DES ，默认3DES

    返回值：无
*/
static void CalcMac_SM4(unsigned char mac[4], const unsigned char* bin, int len, const unsigned char atc[16], const unsigned char key[16]);

//==================================================================================================================================
/*

Hex版本
    SM4算法子密钥推导

     推导算法：参考相关文档说明

     参数：   data   ——（in） 8字节分散系数
              key    ——（in） 密钥 （只能是双倍长密钥）
              divCnt ——（in）分散次数，默认1次

     返回值：分散后的子密钥



*/
static QByteArray  DiversifyKey_SM4(QByteArray data, QByteArray key,int divCnt=1);

//==================================================================================================================================
/*
Hex版本
   SM4算法子密钥推导

   推导算法：参考相关文档说明

   参数：   data     ——（in）8字节分散系数  (实际分散系数16字节： 8字节分散系数 + 8字节分散系数取反）
            key      ——（in）密钥

   返回值：分散后的子密钥

   备注：该函数只支持一次分散，如果需要进行多次分散需要多次调用该函数
*/
static QByteArray  GetSubkey_SM4(QByteArray data, QByteArray key);

//==================================================================================================================================
 /* SM4子密钥推导(bin版本)
    参数：
        rvalue    -(out)子密钥
        rlen      -(out)子密钥长度
        data      -(in) 分散数据
        dlen      -(in) 分散数据长度，必须为8字节倍数
        key       -(in) 密钥

*/
static void  GetSubkey_SM4(unsigned char* rvalue, int& rlen, unsigned char* data, int dlen, unsigned char* key);


//==================================================================================================================================
/* SM4算法过程密钥产生

   推导算法：将输入数据In按位取反得到（~In），按照In||(~In)的顺序连接在一起，组成16字节输入数据DATA；

   参数：   data     ——（in）8字节分散系数  (实际分散系数16字节： 8字节分散系数 + 8字节分散系数取反）
            key      ——（in）密钥

   返回值：过程密钥

*/
static QByteArray  GetSeskey_SM4(QByteArray data, QByteArray key);



//SM4 end
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//==================================================================================================================================
//以下为国密算法 SM2

#ifdef USER_SM2

//------------------------------------------------------------------
//函数名称：GenerateKey_SM2
//函数功能：根据国密SM2算法生成公私钥对
//输入参数：无
//输出参数：pv_key   -私钥（十六进制串）
//          pb_key   -公钥(X||Y)（十六进制串）

//返回值：void
static void __fastcall GenerateKey_SM2(QByteArray& pv_key, QByteArray& pb_key);

//------------------------------------------------------------------
//函数名称：Sign_SM2
//函数功能：国密SM2算法对数据进行签名
//输入参数：msg        -原始报文数据（十六进制值）
//        pv_key       -签名私钥（十六进制值）
//        pb_key       -签名公钥"X||Y"（十六进制值）
//       sig_encoding  -签名结果编码方式: 1-"R||S"   2-DER编码

//输出参数：sign       -签名结果，具体格式由sigcode确定（十六进制值）
//          digest     -摘要结果（对代签名数据按照 SM2密码算法使用规范 的计算结果） 十六进制值

//返回值：int 0表示签名成功，其他表示失败

static int __fastcall Sign_SM2(QByteArray msg, QByteArray pv_key,QByteArray pb_key,int sig_encoding,
							   QByteArray& sign,QByteArray& digest);

//------------------------------------------------------------------
//函数名称：Verify_SM2
//函数功能：国密SM2算法对数据进行验签
//输入参数：msg        -待签名数据（十六进制值）
//        pb_key       -签名公钥"X||Y"（十六进制值）
//        csig         -签名结果，具体格式由sigcode确定（十六进制值）
//        sig_encoding -签名结果编码方式: 1-"R||S"   2-DER编码

//输出参数：无

//返回值： int 0表示验签成功，其他表示失败

static int __fastcall Verify_SM2(QByteArray msg, QByteArray pb_key,QByteArray sign,int sig_encoding=1);


//------------------------------------------------------------------
/*
  SM2加密使用接收方公钥加密，公钥由一个曲线坐标点组成，2个32byte的BigInteger，即曲线点P（x,y）。
  SM2公钥加密结果由3个部分组成，SM2加密过程中使用了随机数，因此同样的明文数据每一次加密结果都不一样。

  根据国密推荐的SM2椭圆曲线公钥密码算法，首先产生随机数，计算出曲线点C1，2个32byte的BigInteger大数，即为SM2加密结果的第1部分。
  第2部分则是真正的密文，是对明文的加密结果，长度和明文一样。
  第3部分是杂凑值，用来校验数据。
  按国密推荐的256位椭圆曲线，明文加密结果比原长度会大96byte。
*/

//函数名称：EncryptData_SM2
//函数功能：国密SM2算法对数据进行加密
//输入参数：plaintext  -待加密的内容（十六进制值）
//          pb_key   -公钥"X||Y"（十六进制值）

//输出参数：ciphertext   -加密后的内容（十六进制值）C1||C3||C2
//          其中：C1-64byte, C2-明文对应密文， C3-杂凑值 -32byte

//返回值：int 0表示成功，其他表示失败
static int __fastcall EncryptData_SM2(QByteArray plaintext, QByteArray pb_key, QByteArray& ciphertext);


//------------------------------------------------------------------
//函数名称：DecryptData_SM2
//函数功能：国密SM2算法对数据进行解密
//输入参数：ciphertext -待解密的密文内容（十六进制值） C1||C3||C2
//          pv_key     -私钥，长度为32字节（十六进制值）


//输出参数：retdata    -解密后的内容（十六进制值）

//返回值：int 0表示成功，其他表示失败
static int __fastcall DecryptData_SM2(QByteArray ciphertext, QByteArray pv_key, QByteArray& plaintext);


#endif


};
#endif
