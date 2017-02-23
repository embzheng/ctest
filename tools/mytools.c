/*****************************************************************************
Copyright (C), Fujian Sunnada Communication. Co., Ltd.
File name    : monitor.c
Description  : 
Author       : zj
Version      :
Date         : 2016年11月15日
Others       : 
History      : 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
1) 日期:      修改者:
   内容:
2）...
 
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>         //AF_INET
#include <netinet/in.h>         //struct in_addr
#include <time.h>

#include "mytools.h"

/*****************************************************************************
 函 数 名  : mac_hex_to_str
 功能描述  : 16进展mac地址转字符串
 输入参数  : u_char *p_mac_hex  
             const char chken   分隔符  default '-'
             u_char *p_mac_str 空间外部申请 
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月15日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void mac_hex_to_str(const u_char * p_mac_hex, const char chken, u_char * p_mac_str)
{
    int     i;
    char    szFormat[] = "%02X-%02X-%02X-%02X-%02X-%02X";

    int     nLen = strlen(szFormat);
    if (chken != '-') {
        for (i = 4; i < nLen; i += 5) {
            szFormat[i] = chken;
        }
    }
    sprintf(p_mac_str, szFormat, p_mac_hex[0], p_mac_hex[1], p_mac_hex[2],
            p_mac_hex[3], p_mac_hex[4], p_mac_hex[5]);
}

u_char char_to_data(const char ch)
{
    switch (ch) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;
    }
    return 0;
}

void mac_str_to_hex(const char *p_mac_str, u_char * p_mac_hex)
{
    int     i;
    const char *pTemp = p_mac_str;
    for (i = 0; i < 6; ++i) {
        p_mac_hex[i] = char_to_data(*pTemp++) * 16;
        p_mac_hex[i] += char_to_data(*pTemp++);
        pTemp++;
    }
}

/*****************************************************************************
 函 数 名  : ip_str_to_hex
 功能描述  : 点分制 ip v4地址转为 整形
 输入参数  : const char *p_ip_str  
             u_char * p_ip_hex  外部分配空间，四字节   
 输出参数  : 无
 返 回 值  : int
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : zj
    修改内容   : 新生成函数

    源函数:
    in_addr_t inet_addr(const char* strptr);
    返回：若字符串有效则将字符串转换为32位二进制网络字节序的IPV4地址，否则为INADDR_NONE

*****************************************************************************/
int ip_str_to_hex(const char *p_ip_str, u_char * p_ip_hex)
{
    int     i_ret;

    i_ret = inet_pton(AF_INET, p_ip_str, p_ip_hex);
    if (i_ret < 0) {
        printf("inet_pton ip:%s err:%s", p_ip_str, strerror(errno));
        return FAILE;
    }

    if (i_ret = 0) {
        printf("ip:%s format err", p_ip_str);
        return FAILE;
    }
    return SUCCESS;
}

/*****************************************************************************
 函 数 名  : ip_hex_to_str
 功能描述  : int 型ip转换为字符串
 输入参数  : const struct in_addr src_ip
             u_char * p_ip_str      外部分配空间，16字节   
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : zj
    修改内容   : 新生成函数

    源函数:char *inet_ntoa (struct in_addr);
           若无错误发生，inet_ntoa()返回一个字符指针。否则的话，返回NULL。
*****************************************************************************/
int ip_hex_to_str(const struct in_addr src_ip, u_char * p_ip_str)
{
    if (inet_ntop(AF_INET, &src_ip, p_ip_str, 16) == NULL) {
        printf("inet_ntop ip:%x err:%s", src_ip, strerror(errno));
        return FAILE;
    }
    return SUCCESS;
}

/*****************************************************************************
 函 数 名  : lr_trim
 功能描述  : 去掉字符串首尾的 \x20 \r \n 字符
 输入参数  : char *s  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void str_lr_trim(char *s)
{
    int     i;
    char   *start = s;
    char   *end;
    int     s_len;

    s_len = strlen(start);
    for (i = 0; i < s_len; i++) {
        if (s[i] == ' ' || s[i] == '\r' || s[i] == '\n') {
            start++;
        } else {
            break;
        }
    }

    s_len = strlen(start);
    end = start + s_len;
    for (i = s_len - 1; i >= 0; i--) {
        if (start[i] == ' ' || start[i] == '\r' || start[i] == '\n') {
            end--;
        } else {
            break;
        }
    }

    memmove(s, start, end - start);
    *end = 0;
}

/*****************************************************************************
 函 数 名  : get_ini_value
 功能描述  : 获取ini文件配置值
 输入参数  : char *profile  文件名(config.ini)
             char *AppName  应用名(mysql)   最大20字节
             char *KeyName  配置字段(host)  最大20字节
             char *KeyVal   配置值(192.168.0.0) 最大50字节
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : zj
    修改内容   : 新生成函数
ini 文件示例 config.ini:
[mysql]
host=192.168.0.0
user=test
password=test
db=testdb
port=3306
*****************************************************************************/
int get_ini_value(char *profile, char *AppName, char *KeyName, char *KeyVal)
{
    char    appname[20], keyname[20];
    char    buf[100], *c;
    FILE   *fp;
    int     found = 0;

    if ((fp = fopen(profile, "r")) == NULL) {
        printf("openfile [%s] error [%s]\n", profile, strerror(errno));
        return FAILE;
    }
    fseek(fp, 0, SEEK_SET);

    sprintf(appname, "[%s]", AppName);
    memset(keyname, 0, sizeof(keyname));

    while (!feof(fp) && fgets(buf, 100, fp) != NULL) {
        //if( l_trim( buf )==0 )
        //continue;

        if (found == 0) {
            if (buf[0] != '[') {
                continue;
            } else if (strncmp(buf, appname, strlen(appname)) == 0) {
                found = 1;
                continue;
            }
        } else if (found == 1) {
            if (buf[0] == '#') {
                continue;
            } else if (buf[0] == '[') {
                break;
            } else {
                if ((c = (char *) strchr(buf, '=')) == NULL)
                    continue;
                memset(keyname, 0, sizeof(keyname));
                sscanf(buf, "%[^=]", keyname);
                if (strcmp(keyname, KeyName) == 0) {
                    sscanf(++c, "%[^\n]", KeyVal);
                    str_lr_trim(KeyVal);
                    found = 2;
                    break;
                } else {
                    continue;
                }
            }
        }
    }

    fclose(fp);

    if (found == 2)
        return SUCCESS;
    else
        return FAILE;
}

/*****************************************************************************
 函 数 名  : str_to_long
 功能描述  : 十进制的字符串格式转为 long int 跳过前面的空格字符，直到遇上数字
             或正负符号才开始做转换，再遇到非数字或字符串结束时('\0')结束转换，
             并将结果返回
 输入参数  : const u_char *src_str  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
long str_to_long(const u_char * src_str)
{
    return strtol(src_str, NULL, 10);
}

/*****************************************************************************
 函 数 名  : get_time
 功能描述  : 获取时间字符串
 输入参数  : u_char *time_out  空间外部分配，不少于32
              BOOL millisecond 是否精确到毫秒
 输出参数  : 无
 返 回 值  : int
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年12月6日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int get_time(u_char * time_out, BOOL millisecond)
{
    struct timeval tv;
    struct tm *pst_tm = NULL;
    char    timestamp[32] = { 0 };

    gettimeofday(&tv, NULL);
    pst_tm = localtime(&tv.tv_sec);

    if (millisecond) {
        snprintf(time_out, 32, "%02d-%02d-%02d %02d:%02d:%02d.%03d",
                 pst_tm->tm_year + 1900,
                 pst_tm->tm_mon + 1, pst_tm->tm_mday,
                 pst_tm->tm_hour, pst_tm->tm_min, pst_tm->tm_sec, tv.tv_usec / 1000);
    } else {
        snprintf(time_out, 32, "%02d-%02d-%02d %02d:%02d:%02d",
                 pst_tm->tm_year + 1900,
                 pst_tm->tm_mon + 1, pst_tm->tm_mday, pst_tm->tm_hour, pst_tm->tm_min,
                 pst_tm->tm_sec);
    }
    return SUCCESS;
}

/*****************************************************************************
 函 数 名  : byte_to_hex_str
 功能描述  : 字节流转换为十六进制字符串
 输入参数  : const unsigned char* source  
             char* dest  空间必须是 source 两倍!!!             
             int sourceLen                
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年12月8日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void byte_to_hex_str(const unsigned char *source, char *dest, int sourceLen)
{
    short   i;
    unsigned char highByte, lowByte;

    for (i = 0; i < sourceLen; i++) {
        highByte = source[i] >> 4;
        lowByte = source[i] & 0x0f;

        highByte += 0x30;

        if (highByte > 0x39)
            dest[i * 2] = highByte + 0x07;
        else
            dest[i * 2] = highByte;

        lowByte += 0x30;
        if (lowByte > 0x39)
            dest[i * 2 + 1] = lowByte + 0x07;
        else
            dest[i * 2 + 1] = lowByte;
    }
    return;
}

/*****************************************************************************
 函 数 名  : hex_str_to_byte
 功能描述  : 十六进制字符串转换为字节流
 输入参数  : const char* source   
             unsigned char* dest  
             int sourceLen        
 输出参数  : 无
 返 回 值  : 转换完成后的长度
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年12月8日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int hex_str_to_byte(const char *source, unsigned char *dest, int sourceLen)
{
    short   i;
    unsigned char highByte, lowByte;

    for (i = 0; i < sourceLen; i += 2) {
        highByte = toupper(source[i]);
        lowByte = toupper(source[i + 1]);

        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;

        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;

        dest[i / 2] = (highByte << 4) | lowByte;
    }
    return sourceLen / 2;
}


