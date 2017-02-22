#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h> //clock_gettime

#include "syd_thread.h"

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
            printf("recv SIGINT\n");    /* �ж� */
            break;
        case SIGTERM:
            printf("recv SIGTERM\n");   /* ��ֹ */
            break;
        case SIGSEGV:
            printf("recv SIGSEGV\n");   /* ��Ч�ڴ����� */
            break;
        case SIGABRT:
            printf("recv SIGABRT\n");   /* �쳣��ֹ */
            break;
        case SIGBUS:
            printf("recv SIGBUS\n");    /* Ӳ������ */
            break;
        case SIGFPE:
            printf("recv SIGFPE\n");    /* �����쳣 */
            break;
        case SIGILL:
            printf("recv SIGILL\n");    /* �Ƿ�Ӳ��ָ�� */
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
   exit(0);
}

struct syd_thread_master *g_master;


int test_timer(struct syd_thread *t)
{
    printf("test timer exipre\n");

    return 0;
}




int main(int argc,char *argv[])
{ 
    int i;
    int i_ret;
    int i_opt;
    struct syd_thread_master *g_master;
    struct syd_thread thread;

    
    struct syd_thread thread_timer;

    g_master = syd_thread_master_create();
    if (g_master == NULL) {
        return -1;
    }

    (void)syd_thread_insert_timer(g_master, &thread_timer, test_timer, NULL, 1, "g-timer");

    while (syd_thread_fetch (g_master, &thread)) {
        syd_thread_call (&thread);
    }

    return 0;


    while ((i_opt = getopt(argc, argv, "hdm:")) != -1) {
        switch (i_opt) {
            case 'h':
                test_usage();
                break;
                break;
            case 'm':

                break;
            default:
                test_usage();
        }
    }
    test_usage();

}
