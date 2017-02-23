/*****************************************************************************
Copyright (C), Fujian Sunnada Communication. Co., Ltd.
File name    : log.h
Description  : log.c 的头文件
Author       : zj
Version      :
Date         : 2013年12月26日
Others       : 
History      : 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
1) 日期:      修改者:
   内容:
2）...
 
*****************************************************************************/

#ifndef __LOG_H__
#define __LOG_H__

#define     LOG_PATH_LEN     128
#define     MAX_LOG_LEN      512
#define     MAX_LOGFILE_LEN  (1024*1024*100)

#define     DBGINFO "[FUN:%s,LINE:%d]"
#define     MDL __func__, __LINE__

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

/* tlv头结构 */
typedef struct log_hdr_s {
    u_char  uc_type;
    u_char  uc_len;             /* data len, contain tlv_hdr */
} __attribute__ ((packed)) log_hdr_t;

#define     LG_TIM                  7
#define     LG_DMP                  6
#define     LG_DBG                  5
#define     LG_INF                  4
#define     LG_WRN                  3
#define     LG_ERR                  2
#define     LG_FTL                  1

#define LOG_MNG_SET_LEVEL           1
#define LOG_MNG_SET_LOG_SIZE        2
#define LOG_MNG_SET_PRINT_FILE      3
#define LOG_MNG_SET_PRINT_STRERR    4
#define LOG_MNG_SET_MILLISEC        5

/*****************************************************************************
 函 数 名  : log_set_path
 功能描述  : 设置log路径
 输入参数  : UCHAR *puc_log_path
 输出参数  : 无
 返 回 值  : 成功返回0 ， 失败返回 -1
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_path(unsigned char *puc_log_path);

/*****************************************************************************
 函 数 名  : log_set_level
 功能描述  : 设置log级别
 输入参数  : UINT ui_level  
 输出参数  : 无
 返 回 值  : 成功返回0 ， 失败返回 -1
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_level(unsigned int ui_level);

/*****************************************************************************
 函 数 名  : log_get_level
 功能描述  : 获取log级别
 输入参数  : UINT ui_level  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月09日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_get_level(u_int * ui_level);

/*****************************************************************************
 函 数 名  : log_set_file_size
 功能描述  : 设置log大小，单位字节
 输入参数  : UINT ui_level  
 输出参数  : 无
 返 回 值  : 成功返回0 ， 失败返回 -1
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_file_size(unsigned int ui_size);

/*****************************************************************************
 函 数 名  : log_set_file_size
 功能描述  : 设置log文件循环个数
 输入参数  : u_char  uc_log_rotate  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_log_rotate(u_char uc_log_rotate);

/*****************************************************************************
 函 数 名  : log_set_print_file
 功能描述  : 设置是否打印到文件
 输入参数  : BOOL print_file  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_print_file(BOOL print_file);

/*****************************************************************************
 函 数 名  : log_set_print_strerr
 功能描述  : 设置是否打印到窗口
 输入参数  : BOOL print_strerr  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_print_strerr(BOOL print_strerr);

/*****************************************************************************
 函 数 名  : log_set_print_procname
 功能描述  : 设置是否打印进程名
 输入参数  : BOOL print_procname  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_print_procname(BOOL print_procname);

/*****************************************************************************
 函 数 名  : log_set_millisecond
 功能描述  : 设置打印精度为微秒
 输入参数  : BOOL millisecond  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_set_millisecond(BOOL millisecond);

/*****************************************************************************
 函 数 名  : log_dump
 功能描述  : 打印buf
 输入参数  : char *txt            
             const u_char *data  
             u_short len              
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void    log_dump(char *txt, const u_char * data, u_short len);

/*****************************************************************************
 函 数 名  : log_dump_force
 功能描述  : 打印buf，不受级别控制
 输入参数  : char *txt            
             const u_char *data  
             int len              
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月24日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void    log_dump_force(char *txt, const u_char * data, u_short len);

/*****************************************************************************
 函 数 名  : log_dump_tobuf
 功能描述  : 把data打印输出到buf
 输入参数  : char *txt           
             const u_char *data  
             u_short len             
             u_char *buf_in  存放输出结果，内存外部申请
             u_short *buf_len    输入buf_in长度，输出打印结果长度    
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年11月15日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void    log_dump_tobuf(char *txt, const u_char * data, u_short len, u_char * buf_in,
                       u_short * buf_len);

/*****************************************************************************
 函 数 名  : log_init
 功能描述  : log 初始化
 输入参数  : u_char * puc_log_path，可为空，空则使用默认路径/var/log/procname.log
             u_int ui_level         log 级别设置      
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月31日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_init(u_char * puc_log_path, u_int ui_level);

/*****************************************************************************
 函 数 名  : log_init_para
 功能描述  : log初始化
 输入参数  : u_char * puc_log_path，可为空，空则使用默认路径/var/log/procname.log  
             u_int ui_level         log 级别设置    
             BOOL print_file        设置是否写文件
             BOOL print_strerr      设置是否打印到窗口(该打印比较耗时，一条10字符log平均耗时0.2ms)
             BOOL millisecond       设置log精度是否要毫秒级
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月31日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int     log_init_para(u_char * puc_log_path, u_int ui_level, BOOL print_file, BOOL print_strerr,
                      BOOL millisecond);

#define log(level, args...) log_msg(level, args );

/*****************************************************************************
 函 数 名  : log_msg_raw
 功能描述  : log的打印函数，不自动换行,无其他打印
 输入参数  : unsigned int ui_level  
             char *fmt              
             ...                    
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月09日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
#define log_raw(level, args...) log_msg_raw(level, args );

#endif                          /* __LOG_H__ */
