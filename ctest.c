#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <locale.h> //LC_ALL

#include "unicode.h"

#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef FAILE
#define FAILE 0
#endif


static void signal_exit(int sigmun)
{
    switch (sigmun) {
        case SIGINT:
            printf("recv SIGINT\n");    /* 中断 */
            break;
        case SIGTERM:
            printf("recv SIGTERM\n");   /* 终止 */
            break;
        case SIGSEGV:
            printf("recv SIGSEGV\n");   /* 无效内存引用 */
            break;
        case SIGABRT:
            printf("recv SIGABRT\n");   /* 异常终止 */
            break;
        case SIGBUS:
            printf("recv SIGBUS\n");    /* 硬件错误 */
            break;
        case SIGFPE:
            printf("recv SIGFPE\n");    /* 算术异常 */
            break;
        case SIGILL:
            printf("recv SIGILL\n");    /* 非法硬件指令 */
            break;
        default:
            printf("recv %d signal\n", sigmun);
            break;
    }

    /* release resource here */

    exit(1);
}

static void test_usage(void)
{
   printf("usage: cxx_test [-hd]                            \n");
   printf("       -h                    print help messages\n");
   printf("       -d                    big lit duan check\n");
   exit(0);
}

void signal_test(void)
{
    char *a = (char *)malloc(5);
    strcpy(a, "a");
    printf("a:%x %x", a, *a);
    free(a);
    printf("a:%x %x", a, *a);
    free(a);
    printf("a:%x %x", a, *a);
    printf("a:%x %x", a, *a);
}


void big_lit_duan()
{
    union utype
    {
        int i;
        char a;
    };
    union utype u;
    u.i = 1;

    if (u.a)
        printf("lit duan\n");
    else
        printf("big duan\n");
    exit(0);
}


//去掉字符串首尾空格函数
void lr_trim(char* s)
{
    int i;
    char *start =s;
    char *end;    
    int s_len;

    s_len=strlen(start);
    for(i=0;i<s_len ;i++)
    {
         if(s[i]==' ' || s[i]=='\r' || s[i]=='\n')
         {
             start++;
         } else {
             break;
         }
    }

    s_len=strlen(start);
    end = start + s_len;
    for(i=s_len-1;i>=0;i--)
    {
         if(start[i]==' ' || start[i]=='\r' || start[i]=='\n')
         {
             end--;
         } else {
             break;
         }
    }

    memmove(s, start, end - start);
    *end = 0;
}

/*****************************************************************************
 * 将一个字符的Unicode(UCS-2和UCS-4)编码转换成UTF-8编码.
 *
 * 参数:
 *    unic     字符的Unicode编码值
 *    pOutput  指向输出的用于存储UTF8编码值的缓冲区的指针
 *    outsize  pOutput缓冲的大小
 *
 * 返回值:
 *    返回转换后的字符的UTF8编码所占的字节数, 如果出错则返回 0 .
 *
 * 注意:
 *     1. UTF8没有字节序问题, 但是Unicode有字节序要求;
 *        字节序分为大端(Big Endian)和小端(Little Endian)两种;
 *        在Intel处理器中采用小端法表示, 在此采用小端法表示. (低地址存低位)
 *     2. 请保证 pOutput 缓冲区有最少有 6 字节的空间大小!
 ****************************************************************************/
int enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput,
        int outSize)
{
    //assert(pOutput != NULL);
    //assert(outSize >= 6);

    if ( unic <= 0x0000007F )
    {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *pOutput     = (unic & 0x7F);
        return 1;
    }
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )
    {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        *(pOutput+1) = (unic & 0x3F) | 0x80;
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;
        return 2;
    }
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )
    {
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
        *(pOutput+2) = (unic & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;
        return 3;
    }
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )
    {
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+3) = (unic & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;
        return 4;
    }
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )
    {
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+4) = (unic & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;
        return 5;
    }
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )
    {
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+5) = (unic & 0x3F) | 0x80;
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;
        return 6;
    }

    return 0;
}

#include <iconv.h>
#define OUTLEN 255


iconv_test()
{
    //char *in_utf8 = "姝ｅ?ㄥ??瑁?";涓浗鎮ㄥソ
    char *in_utf8 = "涓浗鎮ㄥソ";
    char *in_gb2312 = "正在安装";
    char out[OUTLEN];
    int rc;
    int i_ret;
    int i;

    //i_ret = utf82gbk(out, in_utf8, OUTLEN);
    //printf("utf82gbk:%s i_ret:%d \n", out, i_ret);

    //unicode码转为gb2312码
    rc = code_convert("utf-8", "gb2312", in_utf8, (size_t)strlen(in_utf8), out, (size_t)OUTLEN);  
    printf("unicode-->gb2312 out=%sn  src:%s   rc:%d  out[0]:%d out[1]:%d out[2]:%d out[3]:%d \n",out, in_utf8 , rc, out[0], out[1], out[2], out[3]);
    //gb2312码转为unicode码
    rc = g2u(in_gb2312,(size_t)strlen(in_gb2312),out, (size_t)OUTLEN);
    printf("gb2312-->unicode out=%sn  src:%s   rc:%d\n",out, in_gb2312, rc);

    for (i = 0; i < sizeof(wGBKs) ; i++) {
        
    }
}
//代码转换:从一种编码转为另一种编码
int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen,
                    char *outbuf, size_t outlen)
{
    iconv_t cd;
    int rc;
    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);
    //log(LG_ERR, "cd :%d", cd );

    if (cd == 0) {        
        return -1;
    }
        
    memset(outbuf, 0, outlen);
    if (iconv(cd, pin, &inlen, pout, &outlen) == -1) {
        //log(LG_ERR, "iconv err:%s", strerror(errno));
        return -1;
    }
        
    iconv_close(cd);
    *pout = '\0';  
    return 0;
}

//UNICODE码转为GB2312码
int u2g(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
    return code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
}
//GB2312码转为UNICODE码
int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
    return code_convert("gb2312","utf-8",inbuf,inlen,outbuf,outlen);
}

int  u2gbk(char *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
    return code_convert("GBK", "UTF-8", inbuf, inlen, outbuf, outlen);
}
/** 
 * DESCRIPTION: 实现由utf8编码到gbk编码的转换 
 * 
 * Input: gbkStr,转换后的字符串;  srcStr,待转换的字符串; maxGbkStrlen, gbkStr的最 
 大长度 
 * Output: gbkStr 
 * Returns: -1,fail;>0,success 
 * 
 */  
int utf82gbk(char *gbkStr, const char *srcStr, int maxGbkStrlen) {  
    if (NULL == srcStr) {  
        printf("Bad Parameter\n");  
        return -1;  
    }  
  
    //首先先将utf8编码转换为unicode编码  
    if (NULL == setlocale(LC_ALL, "zh_CN.utf8")) //设置转换为unicode前的码,当前为utf8编码  
            {  
        printf("Bad Parameter\n");  
        return -1;  
    }  
  
    int unicodeLen = mbstowcs(NULL, srcStr, 0); //计算转换后的长度  
    if (unicodeLen <= 0) {  
        printf("Can not Transfer!!!\n");  
        return -1;  
    }  
    wchar_t *unicodeStr = (wchar_t *) calloc(sizeof(wchar_t), unicodeLen + 1);  
    mbstowcs(unicodeStr, srcStr, strlen(srcStr)); //将utf8转换为unicode  
  
    //将unicode编码转换为gbk编码  
    if (NULL == setlocale(LC_ALL, "zh_CN.gbk")) //设置unicode转换后的码,当前为gbk  
            {  
        printf("Bad Parameter\n");  
        return -1;  
    }  
    int gbkLen = wcstombs(NULL, unicodeStr, 0); //计算转换后的长度  
    if (gbkLen <= 0) {  
        printf("Can not Transfer!!!\n");  
        return -1;  
    } else if (gbkLen >= maxGbkStrlen) //判断空间是否足够  
            {  
        printf("Dst Str memory not enough\n");  
        return -1;  
    }  
    wcstombs(gbkStr, unicodeStr, gbkLen);  
    gbkStr[gbkLen] = 0; //添加结束符  
    free(unicodeStr);  
    return gbkLen;  
}  
int code_convert2(char *from_charset, char *to_charset, char *inbuf, size_t inlen,  
        char *outbuf, size_t outlen) {  
    iconv_t cd;  
    char **pin = &inbuf;  
    char **pout = &outbuf;  
  
    cd = iconv_open(to_charset, from_charset);  
    if (cd == 0)  
        return -1;  
    memset(outbuf, 0, outlen);  
    if (iconv(cd, pin, &inlen, pout, &outlen) == -1)  
        return -1;  
    iconv_close(cd);  
    *pout = '\0';  
  
    return 0;  
}  
  
int u2g2(char *inbuf, size_t inlen, char *outbuf, size_t outlen) {  
    return code_convert2("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);  
}  
  
int g2u2(char *inbuf, size_t inlen, char *outbuf, size_t outlen) {  
    return code_convert2("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);  
}  
  
int utf8gbktest(void) {  
    char *s = "中国";  
    char buf[10];  
    u2g2(s, strlen(s), buf, sizeof(buf));  
    printf("buf:%s %x %x %x %x \n", buf, buf[0], buf[1], buf[2], buf[3]);
    
    char buf2[10];  
    g2u2(buf, strlen(buf), buf2, sizeof(buf2));  
    printf("buf2:%s \n", buf2);

    return 1;  
} 


#define BigLittleSwap16(A)  ((((u_short)(A) & 0xff00) >> 8) | (((u_short)(A) & 0x00ff) << 8))


#define MAX_KEY_LEN 20
typedef struct str_cmp_s {
    u_char *p_key;
    u_char *p_find;
}str_cmp_t;

str_cmp_t src_key_array[] = {
    {"uin=", 0},
    {"weixin", 0},   
    {0, 0},   
};

int my_strstr(u_char *src_str, str_cmp_t *p_src_key)
{
    int i; //记录源字符串
    int j; //
    int k;
    int temp;
    int flag = 0;

    if (src_str == NULL) {
        return FAILE;
    }

    if (p_src_key == NULL) {
        return FAILE;
    }
    
	for(i = 0; src_str[i] != '\0'; i++)
	{		
	    printf("src_str[i:%d]:%c \n", i, src_str[i]);
        for (j = 0; p_src_key[j].p_key != NULL; j++) {
            if (p_src_key[j].p_find != NULL) {
                continue;
            }
            temp = i; 
            k = 0;            
            printf("src_str[temp:%d]:%c p_src_key[j:%d].p_key[k:%d]:%c \n", 
                temp, src_str[i], j, k, p_src_key[j].p_key[k]);
    		while(src_str[temp] != '\0' && src_str[temp++] == p_src_key[j].p_key[k++])
    		{
    			if(p_src_key[j].p_key[k] == '\0')
    			{
    				p_src_key[j].p_find = &src_str[i];
                    flag = 1;
                    printf("p_src_key[j:%d].p_find:%s \n", j, p_src_key[j].p_find);
                    break;
    			}
    		}
        }		
	}

    if (flag == 1) {
        return SUCCESS;
    }
	return FAILE;
}


int main(int argc,char *argv[])
{ 
    int i;
    int i_ret;
    int i_opt;
    char str[] = "uin=2915927329&clientversion=637734195&scene=0&net=2&md5=02d8691b08787fbbb9fd3ba88c887619&devicetype=android-18&lan=zh_CN&sigver=2 HTTP/1.0  Accept: */*  Accept-Encoding: deflate  Cache-Control: no-cache  Connection: close  Content-Type: application/octet-stream  Host: dns.weixin.qq.com  User-Agent: \n\ra";
    char buf[500];
    char *start;
    char my_str[] = "weixinuain=weixinuian=aweixin";

    

    i_ret = my_strstr(my_str, src_key_array);
    printf("i_ret:%d\n", i_ret);
    for (i = 0; src_key_array[i].p_key != NULL; i++) {
        printf("src_key_array[i:%d].p_find:%s \n", i, src_key_array[i].p_find);
    }

    //signal(SIGABRT, signal_exit);
    //signal(SIGBUS, signal_exit);
    //signal(SIGFPE, signal_exit);
    //signal(SIGILL, signal_exit);
    //signal(SIGSEGV, signal_exit);
    //signal(SIGINT, signal_exit);
    //signal(SIGTERM, signal_exit);

#if 0
    //lr_trim(str);
    memset(buf, 0, 100);

    start = strstr(str, "uin=");
    start += strlen("uin=");
    printf("start:%s", start);
    sscanf(start, "%30[0-9]", buf);
    printf("buf:%s  len:%d  value:%ld \n", buf, strlen(buf), strtol(buf, NULL, 10));

    start = strstr(str, "User-Agent:");
    start += strlen("User-Agent:");
    printf("start:%s", start);
    //sscanf(start, "%30[0-9]", buf);
    sscanf(start, "%255[^\r\n]", buf);
    printf("buf:%s  len:%d  value:%ld \n", buf, strlen(buf), strtol(buf, NULL, 10));
    printf("buf[0]:%x  buf[1]:%x buf[2]:%x ", buf[0], buf[1], buf[2]);
#endif    
#if 0

    char str2[210];
    wchar_t wstr[] = {0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,
    0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002};
    setlocale(LC_ALL, "");
    wcstombs(str2, wstr, sizeof(str2)/sizeof(char));
    printf("%s\n", str2);

    
        
    memset(str2, 0, sizeof(str2));
    i_ret = enc_unicode_to_utf8_one(wstr[0],str2, 6 );
    printf("result i_ret:%d , str2:%s", i_ret, str2);

    iconv_test();
    //iconv_test2();
    //utf8gbktest();
    u_int out_size = 0;
    u_int str_offset = 0;
    u_short unicode_data_byte = 0;
    char    msg[OUTLEN];
    unsigned short     in_gbk[OUTLEN];
    unsigned short     in_utf8ctest[OUTLEN];
    unsigned short     in_unicode[OUTLEN];
    int i = 0;
    int j = 0;
    int k = 0;

    char *in_utf8 = "涓浗鎮ㄥソ";
    char *in_gb2312 = "正在安装";

    strcpy(in_gbk, in_gb2312);
    printf("in_gbk %X %X %X %X %s\n", in_gbk[0], in_gbk[1], in_gbk[2], in_gbk[3], in_gbk);

    memset(in_unicode, 0, sizeof(in_unicode));

    for (i = 0; i < sizeof(in_gbk)/sizeof(u_short); i++) {
        while ( in_gbk[i] != wGBKs[j] && j < (sizeof(wGBKs)/sizeof(u_short)))
        {
            j++;
        } 
        if (j != (sizeof(wGBKs)/sizeof(u_short))) {
            
            in_unicode[k++] = wUnicodes[j];
            printf("j:%d \n", j);   
            printf("wUnicodes[j]:%X  wGBKs[j]:%X  in_unicode:%X  in_unicode:%s \n", wUnicodes[j], wGBKs[j], in_unicode[k-1], &in_unicode[k-1]);  
            //printf("wUnicodes[j]:%X \n", wUnicodes[j]);  
            //printf("wGBKs[j]:%s \n", &wGBKs[j]); 
            //printf("in_gbk:%s \n", &in_gbk[k]);
            //printf("in_gbk:%s \n", &in_gbk[k-1]);
            //printf("in_gbk x:%x \n", in_gbk);
            //printf("in_gbk x:%x \n", in_gbk[k-1]);
            j = 0;
        }        
    } 

    memset(in_gbk, 0, sizeof(in_gbk));

     u_short wstr[] = {0x4E09,0x5143,0x8FBE,0x3002};//,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,
//0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002,0x8FBE,0x3002,0x4E09,0x5143,0x8FBE,0x3002};
   
    printf("sizeof(wUnicodes):%d \n", sizeof(wUnicodes));
    printf("sizeof(wGBKs):%d \n", sizeof(wGBKs));
    printf("sizeof(wstr):%d \n", sizeof(wstr));
    //printf("wGBKs:%x \n", wGBKs[0]);
    //printf("wGBKs:%s \n", wGBKs);
    static const unsigned char sms_msg[] = {
        0x4E, 0x2D, 0x56, 0xFD, 0x60, 0xA8, 0x59, 0x7D
    };

    for (i = 0; i < sizeof(sms_msg)/sizeof(u_short); i++) {
        unicode_data_byte = *(u_short *)&sms_msg[i * 2];
        //unicode_data_byte = BigLittleSwap16(unicode_data_byte);
        printf("unicode_data_byte:%x \n", unicode_data_byte);        
        out_size = enc_unicode_to_utf8_one(unicode_data_byte, &in_utf8ctest[str_offset], OUTLEN);
        str_offset += out_size;
    } 
    printf("enc_unicode_to_utf8_one  in_gbk:%s \n", in_gbk);

    
    u_short us_tmp;
    i =0;
    j = 0;
    k = 0;
    for (i = 0; i < sizeof(sms_msg); i += 2) {
        us_tmp = *(u_short *)&sms_msg[i];
        us_tmp = BigLittleSwap16(us_tmp);
        printf("us_tmp:%X \n", us_tmp);
        while ( us_tmp != wUnicodes[j] && j < (sizeof(wUnicodes)/sizeof(u_short)))
        {
            j++;
        } 
        if (j != (sizeof(wUnicodes)/sizeof(u_short))) {
            
            in_gbk[k++] = wGBKs[j];
            printf("j:%d \n", j);   
            printf("wUnicodes[j]:%X  wGBKs[j]:%X  in_gbk:%X  in_gbk:%s              \n", wUnicodes[j], wGBKs[j], in_gbk[k-1], &in_gbk[k-1]);  
            //printf("wUnicodes[j]:%X \n", wUnicodes[j]);  
            //printf("wGBKs[j]:%s \n", &wGBKs[j]); 
            //printf("in_gbk:%s \n", &in_gbk[k]);
            //printf("in_gbk:%s \n", &in_gbk[k-1]);
            //printf("in_gbk x:%x \n", in_gbk);
            //printf("in_gbk x:%x \n", in_gbk[k-1]);
            j = 0;
        }        
    } 

    printf("in_gbk:%s \n", in_gbk);
#endif

    while ((i_opt = getopt(argc, argv, "hdm:")) != -1) {
        switch (i_opt) {
            case 'h':
                test_usage();
                break;
            case 'd':
                big_lit_duan();
                break;
            case 'm':

                break;
            default:
                test_usage();
        }
    }
    test_usage();

}
