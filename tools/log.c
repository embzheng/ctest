/*****************************************************************************
Copyright (C), Fujian Sunnada Communication. Co., Ltd.
File name    : log.c
Description  : 
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>

#include "log.h"

/*****************************************************************************
             局部宏定义                                                       
*****************************************************************************/

#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef FAILE
#define FAILE 0
#endif

#define UDP_MAX_RCVBUF_LEN  (65535 - 20 - 8)
#define MAX_VALUE_LEN       128 /* 最大值长度 */

int     g_log_socketid = -1;
u_char  procname[100] = "";     //进程名

/*****************************************************************************
             外部变量定义                                                     
*****************************************************************************/

/*****************************************************************************
             全局变量定义                                                     
*****************************************************************************/

/*****************************************************************************
             局部函数声明                                                     
*****************************************************************************/

/*****************************************************************************
             局部表定义                                                       
*****************************************************************************/
typedef struct log_cfg_s {
    u_char  auc_log_path[LOG_PATH_LEN];
    u_int   ui_log_size;
    u_char  uc_log_rotate;      /* log文件循环个数 */
    u_int   ui_log_level;
    BOOL    print_file;
    BOOL    print_strerr;
    BOOL    print_procname;
    BOOL    millisecond;
} log_cfg_t;

/*****************************************************************************
             文件全局变量定义                                                     
*****************************************************************************/
log_cfg_t gst_log_cfg = {
    "/var/log/default.log", 1024 * 1024 * 10, 10, LG_INF, TRUE, TRUE, FALSE, FALSE
};

/*============================================================================
             局部函数                                                         
============================================================================*/
static int log_open_file(const char *device, int mode)
{
    int     m, f, fd = -1;
    m = mode | O_NONBLOCK;
    /* Retry up to 5 times */
    for (f = 0; f < 5; f++) {
        if ((fd = open(device, m, 0600)) >= 0) {
            break;
        }
    }

    if (fd < 0) {
        return fd;
    }

    /* Reset original flags. */
    if (m != mode) {
        fcntl(fd, F_SETFL, mode);
    }

    return fd;

}

/*============================================================================
             全局函数                                                         
============================================================================*/
/*****************************************************************************
 函 数 名  : log_msg_print
 功能描述  : 按print格式打印到文件
 输入参数  : char *fmt  
             ...        
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void    log_msg_print(char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void log_msg_print(char *fmt, ...)
{
    int     fd;
    struct flock fl;
    va_list arguments;
    struct stat statf;
    int     r;
    int     i;
    char    old_file[(strlen(gst_log_cfg.auc_log_path) + 4)];
    char    new_file[(strlen(gst_log_cfg.auc_log_path) + 4)];

    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 1;
    if ((fd =
         log_open_file(gst_log_cfg.auc_log_path,
                       O_WRONLY | O_CREAT | O_NOCTTY | O_APPEND | O_NONBLOCK)) >= 0) {

        fl.l_type = F_WRLCK;
        fcntl(fd, F_SETLKW, &fl);
        if (gst_log_cfg.ui_log_size > 0) {
            r = fstat(fd, &statf);
            if (!r && (statf.st_mode & S_IFREG)
                && (lseek(fd, 0, SEEK_END) > gst_log_cfg.ui_log_size)) {
                if (gst_log_cfg.uc_log_rotate > 0) {
                    for (i = gst_log_cfg.uc_log_rotate - 1; i > 0; i--) {
                        sprintf(old_file, "%s.%d", gst_log_cfg.auc_log_path, i - 1);
                        sprintf(new_file, "%s.%d", gst_log_cfg.auc_log_path, i);
                        rename(old_file, new_file);
                    }

                    sprintf(new_file, "%s.%d", gst_log_cfg.auc_log_path, 0);
                    fl.l_type = F_UNLCK;
                    fcntl(fd, F_SETLKW, &fl);
                    close(fd);
                    rename(gst_log_cfg.auc_log_path, new_file);
                    fd = log_open_file(gst_log_cfg.auc_log_path,
                                       O_WRONLY | O_CREAT | O_NOCTTY | O_APPEND | O_NONBLOCK);
                    fl.l_type = F_WRLCK;

                    fcntl(fd, F_SETLKW, &fl);
                } else {
                    ftruncate(fd, 0);
                }
            }
        }
        va_start(arguments, fmt);
        vdprintf(fd, fmt, arguments);
        va_end(arguments);
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &fl);
        close(fd);
    } else {
        perror("can not open message log file!\n");
    }
}

/*****************************************************************************
 函 数 名  : log_get_process_name
 功能描述  : 获取运行该函数进程名
 输入参数  : char *process_name  : 调用者分配的空间
             int len             : 调用者分配的空间大小
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int log_get_process_name(char *process_name, int len)
{
    int     count = 0;
    int     i;
    char    pro_buf[100];
    char    process[100];
    char   *p_tmp_buf = pro_buf;
    int     tmp_len;
    pid_t   pid = getpid();

    if (!process_name) {
        printf("log_get_process_name process_name is null!\n");
        return -1;
    }

    memset(pro_buf, 0, 100);
    memset(process, 0, 100);
    sprintf(process, "/proc/%d/exe", pid);
    count = readlink(process, pro_buf, 100);

    if (count < 0 || count >= 100) {
        printf("log_get_process_name readlink err!count(%d)\n", count);
        return -1;
    }

    for (i = count - 1; i >= 0; i--) {
        /* 筛选出进程名 */
        if (pro_buf[i] == '/') {
            i++;
            p_tmp_buf += i;
            break;
        }
    }

    tmp_len = strlen(p_tmp_buf);
    if (0 == tmp_len) {
        printf("log_get_process_name Get task fail.\n");
        return -1;
    }

    if (len <= tmp_len + 1) {
        printf("len(%d) is less than taskname(%s)'s len.\n", len, p_tmp_buf);
        return -1;
    }

    strcpy(process_name, p_tmp_buf);

    return 0;

}

/*****************************************************************************
 函 数 名  : log_msg
 功能描述  : log的打印函数，打印到文件
 输入参数  : unsigned int ui_level  
             char *fmt              
             ...                    
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void log_msg(unsigned int ui_level, char *fmt, ...)
{
    char    log_buf[MAX_LOG_LEN];
    va_list ap;
    struct timeval tv;
    struct tm *pst_tm = NULL;
    char    timestamp[32] = { 0 };
    int     i_ret;
    char   *p_buf_tmp;

    if (ui_level > gst_log_cfg.ui_log_level) {
        return;
    }

    gettimeofday(&tv, NULL);
    pst_tm = localtime(&tv.tv_sec);
    va_start(ap, fmt);
    vsnprintf(log_buf, 512, fmt, ap);
    va_end(ap);
    /* 去除换行符 */
    while (p_buf_tmp = strchr(log_buf, '\n')) {
        *p_buf_tmp = ' ';
    }

    while (p_buf_tmp = strchr(log_buf, '\r')) {
        *p_buf_tmp = ' ';
    }

    if (gst_log_cfg.millisecond) {
        snprintf(timestamp, sizeof(timestamp), "%02d-%02d-%02d %02d:%02d:%02d.%03d",
                 pst_tm->tm_year + 1900,
                 pst_tm->tm_mon + 1, pst_tm->tm_mday,
                 pst_tm->tm_hour, pst_tm->tm_min, pst_tm->tm_sec, tv.tv_usec / 1000);
    } else {
        snprintf(timestamp, sizeof(timestamp), "%02d-%02d-%02d %02d:%02d:%02d",
                 pst_tm->tm_year + 1900,
                 pst_tm->tm_mon + 1, pst_tm->tm_mday, pst_tm->tm_hour, pst_tm->tm_min,
                 pst_tm->tm_sec);
    }

    if (gst_log_cfg.print_procname) {
        if (strlen(procname) == 0) {
            log_get_process_name(procname, 100);
        }

        if (gst_log_cfg.print_file) {
            log_msg_print("[%s]%s-%d-%s\n", timestamp, procname, ui_level, log_buf);
        }
        if (gst_log_cfg.print_strerr) {
            fprintf(stderr, "[%s]%s-%d-%s\n", timestamp, procname, ui_level, log_buf);
        }
    } else {
        if (gst_log_cfg.print_file) {
            log_msg_print("[%s]-%d-%s\n", timestamp, ui_level, log_buf);
        }
        if (gst_log_cfg.print_strerr) {
            fprintf(stderr, "[%s]-%d-%s\n", timestamp, ui_level, log_buf);
        }
    }
}

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
void log_msg_raw(unsigned int ui_level, char *fmt, ...)
{
    char    log_buf[MAX_LOG_LEN];
    va_list ap;
    int     i_ret;
    char   *p_buf_tmp;

    if (ui_level > gst_log_cfg.ui_log_level) {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(log_buf, 512, fmt, ap);
    va_end(ap);

    if (gst_log_cfg.print_file) {
        log_msg_print("%s", log_buf);
    }

    if (gst_log_cfg.print_strerr) {
        fprintf(stderr, "%s", log_buf);
    }
}

/*****************************************************************************
 函 数 名  : log_set_path
 功能描述  : 设置log路径
 输入参数  : UCHAR *puc_log_path
 输出参数  : 无
 返 回 值  : INT
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int log_set_path(u_char * puc_log_path)
{

    if (!puc_log_path) {
        log_msg(LG_ERR, DBGINFO "puc_log_path is null!", MDL);
        return -1;
    }

    if (!strlen(puc_log_path)) {
        log_msg(LG_ERR, DBGINFO "puc_log_path is str_null!", MDL);
        return -1;
    }

    strcpy(gst_log_cfg.auc_log_path, puc_log_path);
    log_msg(LG_FTL, "set log path:%s", gst_log_cfg.auc_log_path);
    return 0;
}

/*****************************************************************************
 函 数 名  : log_set_level
 功能描述  : 设置log级别
 输入参数  : UINT ui_level  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int log_set_level(u_int ui_level)
{
    if (ui_level < LG_FTL || ui_level > LG_TIM) {
        log_msg(LG_FTL, DBGINFO "ui_level(%d) is invalid!", MDL, ui_level);
        return -1;
    }

    gst_log_cfg.ui_log_level = ui_level;
    log_msg(LG_FTL, "set log_level:%d success!", ui_level);
    return 0;
}

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
int log_get_level(u_int * ui_level)
{
    if (NULL == ui_level) {
        log_msg(LG_ERR, DBGINFO "ui_level is null", MDL);
        return -1;
    }

    *ui_level = gst_log_cfg.ui_log_level;
    return 0;
}

/*****************************************************************************
 函 数 名  : log_set_file_size
 功能描述  : 设置log大小，单位字节
 输入参数  : UINT ui_level  
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2013年12月26日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int log_set_file_size(u_int ui_size)
{
    if (ui_size <= 0 || ui_size > MAX_LOGFILE_LEN) {
        log_msg(LG_ERR, DBGINFO "ui_size(%d) is invalid!", MDL, ui_size);
        return -1;
    }

    gst_log_cfg.ui_log_size = ui_size;
    log_msg(LG_FTL, "set log size:%d", ui_size);
    return 0;
}

/*****************************************************************************
 函 数 名  : log_set_log_rotate
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
int log_set_log_rotate(u_char uc_log_rotate)
{
    gst_log_cfg.uc_log_rotate = uc_log_rotate;
    log_msg(LG_FTL, "set uc_log_rotate:%d", uc_log_rotate);
    return 0;
}

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
int log_set_print_file(BOOL print_file)
{
    if (gst_log_cfg.print_strerr == FALSE && print_file == FALSE) {
        log_msg(LG_ERR, "can not set print_file false!");
    } else {
        gst_log_cfg.print_file = print_file;
        log_msg(LG_FTL, "set print_file:%d", print_file);
    }
    return 0;
}

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
int log_set_print_strerr(BOOL print_strerr)
{
    if (gst_log_cfg.print_file == FALSE && print_strerr == FALSE) {
        log_msg(LG_ERR, "can not set print_strerr false!");
    } else {
        gst_log_cfg.print_strerr = print_strerr;
        log_msg(LG_FTL, "set print_strerr:%d", print_strerr);
    }
    return 0;
}

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
int log_set_print_procname(BOOL print_procname)
{
    gst_log_cfg.print_procname = print_procname;
    log_msg(LG_FTL, "set print_procname:%d", print_procname);
    return 0;
}

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
int log_set_millisecond(BOOL millisecond)
{
    gst_log_cfg.millisecond = millisecond;
    log_msg(LG_FTL, "set millisecond:%d", millisecond);
    return 0;
}

/*****************************************************************************
 函 数 名  : log_dump
 功能描述  : buf打印
 输入参数  : char *txt            
             const u_char *data  
             u_short len              
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年8月16日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void log_dump(char *txt, const u_char * data, u_short len)
{
    int     i, j;
    char    buf[100];
    int     l;
    if (len == 0) {
        return;
    }

    if (gst_log_cfg.ui_log_level < LG_DMP) {
        return;
    }

    log(LG_DMP, "+-------| %s len=%d |-------\n", txt, len);

    memset(buf, 0, sizeof(buf));

    for (i = 0; i < len; i += 16) {
        l = 0;
        l += snprintf(buf + l, sizeof(buf) - l, "%04X  ", i);
        for (j = i; (j < i + 16); j++) {
            if (j < len) {
                l += snprintf(buf + l, sizeof(buf) - l, "%02X ", (unsigned char) *(data + j));
            } else {
                l += snprintf(buf + l, sizeof(buf) - l, "   ");
            }
        }

        l += snprintf(buf + l, sizeof(buf) - l, "  ");

        for (j = i; (j < i + 16) && (j < len); j++) {
            if ((*(data + j) >= 32) && (*(data + j) <= 127)) {
                l += snprintf(buf + l, sizeof(buf) - l, "%c", *(data + j));
            } else {
                l += snprintf(buf + l, sizeof(buf) - l, ".");
            }
        }

        l += snprintf(buf + l, sizeof(buf) - l, "\n");
        log(LG_DMP, "%s", buf);
    }

    return;

}

/*****************************************************************************
 函 数 名  : log_dump_force
 功能描述  : 打印数据，无视log级别
 输入参数  : char *txt            
             const u_char *data  
             u_short len              
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年9月1日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
void log_dump_force(char *txt, const u_char * data, u_short len)
{
    int     i, j;
    char    buf[100];
    int     l;
    if (len == 0) {
        return;
    }

    log(LG_FTL, "+-------| %s len=%d |-------\n", txt, len);

    memset(buf, 0, sizeof(buf));

    for (i = 0; i < len; i += 16) {
        l = 0;
        l += snprintf(buf + l, sizeof(buf) - l, "%04X  ", i);
        for (j = i; (j < i + 16); j++) {
            if (j < len) {
                l += snprintf(buf + l, sizeof(buf) - l, "%02X ", (unsigned char) *(data + j));
            } else {
                l += snprintf(buf + l, sizeof(buf) - l, "   ");
            }
        }

        l += snprintf(buf + l, sizeof(buf) - l, "  ");

        for (j = i; (j < i + 16) && (j < len); j++) {
            if ((*(data + j) >= 32) && (*(data + j) <= 127)) {
                l += snprintf(buf + l, sizeof(buf) - l, "%c", *(data + j));
            } else {
                l += snprintf(buf + l, sizeof(buf) - l, ".");
            }
        }

        l += snprintf(buf + l, sizeof(buf) - l, "\n");
        log(LG_FTL, "%s", buf);
    }

    return;

}

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
void log_dump_tobuf(char *txt, const u_char * data, u_short len, u_char * buf_in, u_short * buf_len)
{
    int     i, j;
    char    buf[100];
    int     l;
    u_int   offset = 0;

    if (len == 0) {
        *buf_len = 0;
        return;
    }

    memset(buf, 0, sizeof(buf));
    memset(buf_in, 0, *buf_len);

    offset += snprintf(buf_in + offset, *buf_len - offset, "%s len=%d \n", txt, len);

    for (i = 0; i < len; i += 16) {
        l = 0;
        l += snprintf(buf + l, sizeof(buf) - l, "%04X  ", i);
        for (j = i; (j < i + 16); j++) {
            if (j < len) {
                l += snprintf(buf + l, sizeof(buf) - l, "%02X ", (unsigned char) *(data + j));
            } else {
                l += snprintf(buf + l, sizeof(buf) - l, "   ");
            }
        }

        l += snprintf(buf + l, sizeof(buf) - l, "  ");

        for (j = i; (j < i + 16) && (j < len); j++) {
            if ((*(data + j) >= 32) && (*(data + j) <= 127)) {
                l += snprintf(buf + l, sizeof(buf) - l, "%c", *(data + j));
            } else {
                l += snprintf(buf + l, sizeof(buf) - l, ".");
            }
        }

        l += snprintf(buf + l, sizeof(buf) - l, "\n");
        offset += snprintf(buf_in + offset, *buf_len - offset, "%s", buf);
    }
    *buf_len = offset;
    return;

}

int log_creat_socket(u_char * p_source_addr)
{
    int     i_ret;
    int     i_socket;
    struct sockaddr_un st_src_addr;

    i_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (i_socket < 0) {
        log(LG_ERR, "creat socket:%s err:%s", p_source_addr, strerror(errno));
        return FAILE;
    }

    if (p_source_addr != NULL && strlen(p_source_addr) != 0) {
        /* 绑定地址 */
        unlink(p_source_addr);
        remove(p_source_addr);
        memset(&st_src_addr, 0, sizeof(st_src_addr));
        st_src_addr.sun_family = AF_UNIX;
        strcpy(st_src_addr.sun_path, p_source_addr);

        i_ret = bind(i_socket, (struct sockaddr *) &st_src_addr, sizeof(st_src_addr));
        if (i_ret < 0) {
            log(LG_ERR, "bind %s err:%s", p_source_addr, strerror(errno));
            return FAILE;
        }
    }
    return i_socket;
}

int log_control_oper(u_char * p_buf, u_short us_buf_len)
{
    log_hdr_t *p_tlv_hdr;
    u_char  value_len = 0;
    u_char *buf_tmp = p_buf;
    u_short len_remain = us_buf_len;
    u_char  set_value[MAX_VALUE_LEN];
    BOOL    bool_value = FALSE;

    /* analy attributes */
    while ((buf_tmp - p_buf) < us_buf_len) {
        if (len_remain < sizeof(log_hdr_t)) {
            log(LG_ERR, "len_remain:%d, head analyse end!", len_remain);
            return FAILE;
        } else {
            log(LG_DBG, "len_remain:%d", len_remain);
        }

        p_tlv_hdr = (log_hdr_t *) buf_tmp;
        buf_tmp += sizeof(log_hdr_t);
        len_remain = us_buf_len - (buf_tmp - p_buf);
        log(LG_DBG, DBGINFO "uc_type:0x%X  uc_len:%d", MDL, p_tlv_hdr->uc_type, p_tlv_hdr->uc_len);

        if (p_tlv_hdr->uc_len < 3) {
            log(LG_ERR, DBGINFO "uc_len(%d) err !!", MDL, p_tlv_hdr->uc_len);
            return FAILE;
        }

        value_len = p_tlv_hdr->uc_len - sizeof(log_hdr_t);
        if (len_remain < value_len) {
            log(LG_ERR, DBGINFO "value_len(%d) err !! len_remain:%d", MDL, value_len, len_remain);
            return FAILE;
        }

        switch (p_tlv_hdr->uc_type) {
        case LOG_MNG_SET_LEVEL:
            if (value_len > MAX_VALUE_LEN) {
                log(LG_ERR, DBGINFO "value_len len(%d) err !!", MDL, value_len);
                return FAILE;
            }
            memset(set_value, 0, sizeof(set_value));
            strncpy(set_value, buf_tmp, value_len);
            log_set_level(atoi(set_value));
            break;

        case LOG_MNG_SET_LOG_SIZE:
            if (value_len > MAX_VALUE_LEN) {
                log(LG_ERR, DBGINFO "value_len len(%d) err !!", MDL, value_len);
                return FAILE;
            }
            memset(set_value, 0, sizeof(set_value));
            strncpy(set_value, buf_tmp, value_len);

            log_set_file_size(atoi(set_value));
            break;

        case LOG_MNG_SET_PRINT_FILE:
            if (value_len > MAX_VALUE_LEN) {
                log(LG_ERR, DBGINFO "value_len len(%d) err !!", MDL, value_len);
                return FAILE;
            }
            memset(set_value, 0, sizeof(set_value));
            strncpy(set_value, buf_tmp, value_len);

            if (atoi(set_value)) {
                bool_value = TRUE;
            } else {
                bool_value = FALSE;
            }
            log_set_print_file(bool_value);
            break;

        case LOG_MNG_SET_PRINT_STRERR:
            if (value_len > MAX_VALUE_LEN) {
                log(LG_ERR, DBGINFO "value_len len(%d) err !!", MDL, value_len);
                return FAILE;
            }
            memset(set_value, 0, sizeof(set_value));
            strncpy(set_value, buf_tmp, value_len);

            if (atoi(set_value)) {
                bool_value = TRUE;
            } else {
                bool_value = FALSE;
            }
            log_set_print_strerr(bool_value);
            break;

        case LOG_MNG_SET_MILLISEC:
            if (value_len > MAX_VALUE_LEN) {
                log(LG_ERR, DBGINFO "value_len len(%d) err !!", MDL, value_len);
                return FAILE;
            }
            memset(set_value, 0, sizeof(set_value));
            strncpy(set_value, buf_tmp, value_len);

            if (atoi(set_value)) {
                bool_value = TRUE;
            } else {
                bool_value = FALSE;
            }
            log_set_millisecond(bool_value);
            break;

        default:
            log(LG_ERR, "unknown type:%d!", p_tlv_hdr->uc_type);

        }
        buf_tmp += value_len;
        len_remain = us_buf_len - (buf_tmp - p_buf);
    }

    return SUCCESS;
}

int log_control_recv(void)
{
    struct sockaddr_un st_src_addr;
    socklen_t addr_len;
    u_char *p_buf;
    int     buf_len;
    int     i_ret;

    p_buf = (u_char *) malloc(UDP_MAX_RCVBUF_LEN);
    if (NULL == p_buf) {
        log(LG_ERR, "malloc buf err:%s", strerror(errno));
        return FAILE;
    }
    memset(p_buf, 0, UDP_MAX_RCVBUF_LEN);
    memset(&st_src_addr, 0, sizeof(struct sockaddr_un));
    addr_len = sizeof(struct sockaddr_un);

    /* 接收数据 */
    buf_len = recvfrom(g_log_socketid, p_buf, UDP_MAX_RCVBUF_LEN, 0,
                       (struct sockaddr *) &st_src_addr, &addr_len);

    if (buf_len < 0) {
        log(LG_ERR, "recv buf err:%s", strerror(errno));
        free(p_buf);
        return FAILE;
    }
    log_dump("log_control_recv", p_buf, buf_len);

    i_ret = log_control_oper(p_buf, buf_len);
    if (i_ret != SUCCESS) {
        log(LG_ERR, "analyze_manage err");
        return FAILE;
    }

    free(p_buf);
    return SUCCESS;
}

int log_control_main(void)
{
    int     i_ret;
    volatile fd_set fdset;
    volatile fd_set g_fdset;
    int     i_max_socketid = 0;
    u_char  server_fd[120];

    memset(server_fd, 0, sizeof(server_fd));
    sprintf(server_fd, "/tmp/%s.logfd", procname);
    g_log_socketid = log_creat_socket(server_fd);
    if (g_log_socketid == FAILE) {
        log(LG_ERR, DBGINFO "creat socket err", MDL);
        return FAILE;
    }

    log(LG_INF, "loglevel:%d, logpatch:%s, logfd:%s",
        gst_log_cfg.ui_log_level, gst_log_cfg.auc_log_path, server_fd);

    FD_ZERO((fd_set *) & g_fdset);
    i_max_socketid = i_max_socketid > g_log_socketid ? i_max_socketid : g_log_socketid;
    FD_SET(g_log_socketid, &g_fdset);

    for (;;) {
        fdset = g_fdset;
        i_ret = select(i_max_socketid + 1, (fd_set *) & fdset, NULL, NULL, NULL);
        if (i_ret > 0) {
            if ((-1 != g_log_socketid) && (1 == FD_ISSET(g_log_socketid, &fdset))) {
                log_control_recv();
            }
        }
    }
}

/*****************************************************************************
 函 数 名  : log_init
 功能描述  : log 初始化
 输入参数  : u_char * puc_log_path，可为空，空则使用默认路径/var/log/procname.log
             u_int ui_level         
 输出参数  : 无
 返 回 值  : 
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2016年10月31日
    作    者   : zj
    修改内容   : 新生成函数

*****************************************************************************/
int log_init(u_char * puc_log_path, u_int ui_level)
{
    int     i_ret;
    pthread_t unix_pthread;
    u_char  log_patch[120];

    memset(log_patch, 0, sizeof(log_patch));

    i_ret = log_get_process_name(procname, 100);
    if (i_ret == -1) {
        log(LG_ERR, "get process name err!\n");
        return FAILE;
    }
    log(LG_DBG, "get process name:%s", procname);

    if (puc_log_path == NULL) {
        sprintf(log_patch, "/var/log/%s.log", procname);
        log_set_path(log_patch);
    } else {
        log_set_path(puc_log_path);
    }

    log_set_level(ui_level);

    i_ret = pthread_create(&unix_pthread, NULL, (void *) log_control_main, NULL);
    if (i_ret == 0) {
        pthread_detach(unix_pthread);
    } else {
        log(LG_ERR, "pthread create faild %d,reason:%s\n", i_ret, strerror(errno));
    }
}

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
int log_init_para(u_char * puc_log_path, u_int ui_level, BOOL print_file, BOOL print_strerr,
                  BOOL millisecond)
{
    int     i_ret;
    pthread_t unix_pthread;
    u_char  log_patch[120];

    memset(log_patch, 0, sizeof(log_patch));

    i_ret = log_get_process_name(procname, 100);
    if (i_ret == -1) {
        log(LG_ERR, "get process name err!\n");
        return FAILE;
    }
    log(LG_DBG, "get process name:%s", procname);

    if (puc_log_path == NULL) {
        sprintf(log_patch, "/var/log/%s.log", procname);
        log_set_path(log_patch);
    } else {
        log_set_path(puc_log_path);
    }

    log_set_level(ui_level);
    log_set_print_file(print_file);
    log_set_print_strerr(print_strerr);
    log_set_millisecond(millisecond);

    i_ret = pthread_create(&unix_pthread, NULL, (void *) log_control_main, NULL);
    if (i_ret == 0) {
        pthread_detach(unix_pthread);
    } else {
        log(LG_ERR, "pthread create faild %d,reason:%s\n", i_ret, strerror(errno));
    }
}
