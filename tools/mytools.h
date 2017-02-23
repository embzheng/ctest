/*****************************************************************************
Copyright (C), Fujian Sunnada Communication. Co., Ltd.
File name    : tlvanalyse.h
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
#ifndef __MYTOOLS_H__
#define __MYTOOLS_H__

#ifndef TYPEDEF_BOOL
#define TYPEDEF_BOOL
typedef unsigned char BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef FAILE
#define FAILE 0
#endif

#define MAX_FILENAME_LEN            64  /* 最大文件名长度 */
#define MAX_NAME_LEN                32  /* 最大字段名长度 */
#define MAX_MAC_STR_LEN             18  /* 最大字符串mac地址长度 */
#define MAX_IP_STR_LEN              64  /* 最大IP地址长度 */

/*****************************************************************************
 函 数 名  : mac_hex_to_str
 功能描述  : 16进展mac地址转字符串
 输入参数  : u_char *p_mac_hex  
             const char chken   分隔符  default ':'
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
void    mac_hex_to_str(const u_char * p_mac_hex, const char chken, u_char * p_mac_str);

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

*****************************************************************************/
int     ip_hex_to_str(const struct in_addr src_ip, u_char * p_ip_str);

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
int     get_ini_value(char *profile, char *AppName, char *KeyName, char *KeyVal);

/*****************************************************************************
 函 数 名  : str_to_long
 功能描述  : 十进制的字符串格式转为 long int 跳过前面的空格字符，直到遇上数字或正负符号才开始做转换，
             再遇到非数字或字符串结束时('\0')结束转换，并将结果返回
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
long    str_to_long(const u_char * src_str);

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
int     get_time(u_char * time_out, BOOL millisecond);

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
int     hex_str_to_byte(const char *source, unsigned char *dest, int sourceLen);


#endif
