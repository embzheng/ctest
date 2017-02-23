/*****************************************************************************
Copyright (C), Fujian Sunnada Communication. Co., Ltd.
File name    : monitor.c
Description  : 
Author       : zj
Version      :
Date         : 2016��11��15��
Others       : 
History      : 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
1) ����:      �޸���:
   ����:
2��...
 
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
 �� �� ��  : mac_hex_to_str
 ��������  : 16��չmac��ַת�ַ���
 �������  : u_char *p_mac_hex  
             const char chken   �ָ���  default '-'
             u_char *p_mac_str �ռ��ⲿ���� 
 �������  : ��
 �� �� ֵ  : 
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��11��15��
    ��    ��   : zj
    �޸�����   : �����ɺ���

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
 �� �� ��  : ip_str_to_hex
 ��������  : ����� ip v4��ַתΪ ����
 �������  : const char *p_ip_str  
             u_char * p_ip_hex  �ⲿ����ռ䣬���ֽ�   
 �������  : ��
 �� �� ֵ  : int
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : zj
    �޸�����   : �����ɺ���

    Դ����:
    in_addr_t inet_addr(const char* strptr);
    ���أ����ַ�����Ч���ַ���ת��Ϊ32λ�����������ֽ����IPV4��ַ������ΪINADDR_NONE

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
 �� �� ��  : ip_hex_to_str
 ��������  : int ��ipת��Ϊ�ַ���
 �������  : const struct in_addr src_ip
             u_char * p_ip_str      �ⲿ����ռ䣬16�ֽ�   
 �������  : ��
 �� �� ֵ  : 
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : zj
    �޸�����   : �����ɺ���

    Դ����:char *inet_ntoa (struct in_addr);
           ���޴�������inet_ntoa()����һ���ַ�ָ�롣����Ļ�������NULL��
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
 �� �� ��  : lr_trim
 ��������  : ȥ���ַ�����β�� \x20 \r \n �ַ�
 �������  : char *s  
 �������  : ��
 �� �� ֵ  : 
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : zj
    �޸�����   : �����ɺ���

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
 �� �� ��  : get_ini_value
 ��������  : ��ȡini�ļ�����ֵ
 �������  : char *profile  �ļ���(config.ini)
             char *AppName  Ӧ����(mysql)   ���20�ֽ�
             char *KeyName  �����ֶ�(host)  ���20�ֽ�
             char *KeyVal   ����ֵ(192.168.0.0) ���50�ֽ�
 �������  : ��
 �� �� ֵ  : 
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : zj
    �޸�����   : �����ɺ���
ini �ļ�ʾ�� config.ini:
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
 �� �� ��  : str_to_long
 ��������  : ʮ���Ƶ��ַ�����ʽתΪ long int ����ǰ��Ŀո��ַ���ֱ����������
             ���������Ųſ�ʼ��ת���������������ֻ��ַ�������ʱ('\0')����ת����
             �����������
 �������  : const u_char *src_str  
 �������  : ��
 �� �� ֵ  : 
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : zj
    �޸�����   : �����ɺ���

*****************************************************************************/
long str_to_long(const u_char * src_str)
{
    return strtol(src_str, NULL, 10);
}

/*****************************************************************************
 �� �� ��  : get_time
 ��������  : ��ȡʱ���ַ���
 �������  : u_char *time_out  �ռ��ⲿ���䣬������32
              BOOL millisecond �Ƿ�ȷ������
 �������  : ��
 �� �� ֵ  : int
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��12��6��
    ��    ��   : zj
    �޸�����   : �����ɺ���

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
 �� �� ��  : byte_to_hex_str
 ��������  : �ֽ���ת��Ϊʮ�������ַ���
 �������  : const unsigned char* source  
             char* dest  �ռ������ source ����!!!             
             int sourceLen                
 �������  : ��
 �� �� ֵ  : 
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��12��8��
    ��    ��   : zj
    �޸�����   : �����ɺ���

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
 �� �� ��  : hex_str_to_byte
 ��������  : ʮ�������ַ���ת��Ϊ�ֽ���
 �������  : const char* source   
             unsigned char* dest  
             int sourceLen        
 �������  : ��
 �� �� ֵ  : ת����ɺ�ĳ���
 ���ú���  : 
 ��������  : 
 
 �޸���ʷ      :
  1.��    ��   : 2016��12��8��
    ��    ��   : zj
    �޸�����   : �����ɺ���

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


