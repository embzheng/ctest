#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>


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
   printf("usage: wakeuponlan [m:n:p:]                            \n");
   printf("       -h                    print help messages\n");
   printf("       -m                    dest mac[1C:6F:65:9E:20:D2]\n");
   //printf("       -n                    dest ip\n");
   //printf("       -p                    dest port\n");
   exit(0);
}

int creat_udp_socket(u_int ipaddr, u_short us_port, int *sock_fd)
{
    int     ser_sockfd;
    socklen_t addrlen;
    struct sockaddr_in ser_addr;

    if (sock_fd == NULL) {
        perror("creat socket err!");
        return -1;
    }

    /*建立socket */
    ser_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ser_sockfd < 0) {
        perror("creat socket err!");
        return -1;
    }

    /*填写sockaddr_in 结构 */
    addrlen = sizeof(struct sockaddr_in);
    bzero(&ser_addr, addrlen);
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(ipaddr);
    ser_addr.sin_port = htons(us_port);

    /* 绑定 */
    if (bind(ser_sockfd, (struct sockaddr *) &ser_addr, addrlen) < 0) {
        perror("set socket bind err");
        return -1;
    }

    *sock_fd = ser_sockfd;

    return 0;
}
#if 0
#define PORT 7773
#define MAXDATASIZE 256

int main(int argc,char *argv[])
{
    int socket_fd;
    struct sockaddr_in my_addr,user_addr;
    char buf[MAXDATASIZE];
    int so_broadcast=1;
    socklen_t size;
    char my_ip[12];

    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(PORT);
    my_addr.sin_addr.s_addr=inet_addr("255.255.255.255");
    bzero(&(my_addr.sin_zero),8);
    
    user_addr.sin_family=AF_INET;
    user_addr.sin_port=htons(PORT);
    user_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    bzero(&(user_addr.sin_zero),8);
    if((socket_fd=(socket(AF_INET,SOCK_DGRAM,0)))==-1) {
        perror("socket");
        exit(1);
    }
   setsockopt(socket_fd,SOL_SOCKET,SO_BROADCAST,&so_broadcast,sizeof(so_broadcast));
    if((bind(socket_fd,(struct sockaddr *)&user_addr,
                        sizeof(struct sockaddr)))==-1) {
        perror("bind");
        exit(1);
    }
    strcpy(buf,"Hello,I'm on line!");
    sendto(socket_fd,buf,strlen(buf),0,(struct sockaddr *)&my_addr,sizeof(my_addr));
    size=sizeof(user_addr);
    recvfrom(socket_fd,buf,MAXDATASIZE,0,(struct sockaddr *)&user_addr,&size);
    strcpy(my_ip,inet_ntoa(user_addr.sin_addr));
    printf("my_ip:%sn",inet_ntoa(user_addr.sin_addr));
    while(1) {
        bzero(buf, sizeof(buf));          
        size=sizeof(user_addr);
        recvfrom(socket_fd,buf,MAXDATASIZE,0,(struct sockaddr *)&user_addr,&size);
        printf("%s:%sn",inet_ntoa(user_addr.sin_addr),buf);
        bzero(buf, sizeof(buf));
        if(strcmp(buf,"I'm off line,bye!")==0)
            strcpy(buf,"ok,I know,bye!");
        else {
            strcpy(buf,"send,Hello,I get you!");
                sleep(1);
        }    
        if((sendto(socket_fd,buf,strlen(buf),0,
                (struct sockaddr *)&user_addr,sizeof(user_addr)))==-1)
            perror("sendto");
    }
    return 0;
}

#endif
int send_udp(u_int ipaddr, u_short us_port, u_char *puc_data, u_short us_len)
{
    int     i_ret;
    int     sockfd;
    socklen_t addrlen;
    struct sockaddr_in sock_addr;
    int so_broadcast=1;

    /*建立socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("creat socket err!");
        return -1;
    }

    /* 设置允许广播 */
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));
    
    /*填写sockaddr_in 结构 */
    addrlen = sizeof(sock_addr);
    bzero(&sock_addr, addrlen);
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(ipaddr);
    sock_addr.sin_port = htons(us_port);   

    i_ret = sendto(sockfd, puc_data, us_len, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if (-1 == i_ret) {
        perror("send err!");
        return -1;
    }
    return 0;    
}

int mac_str_to_bin(char *mac_str, u_char *mac_bin)
{
    if ((mac_bin == NULL) || (mac_str == NULL))
    {
        return -1;
    }
    if (strlen(mac_str) < 17) {
        return -1;
    }
    sscanf(mac_str, "%2x:%2x:%2x:%2x:%2x:%2x", mac_bin, mac_bin + 1, mac_bin + 2, mac_bin + 3,
        mac_bin + 4, mac_bin + 5);

    return 0;
}




int main(int argc,char *argv[])
{   
    int i_ret;
    int i;
    int i_opt;
    u_char mac_str[18];
    u_char mac_bin[6];
    int islan = 1;
    u_char puc_data[200];

    memset(mac_bin, 0, 6);
    memset(mac_str, 0, 18);
    memset(puc_data, 0, 200);  
    
    while ((i_opt = getopt(argc, argv, "m:n:p:")) != -1) {
        switch (i_opt) {
            case 'm':
                strncpy(mac_str, optarg, 17);
                break;
            case 'h':
                test_usage();
                break;
           
            case 'n':                

                break;
            case 'p':

                break;
            
            default:
                test_usage();
        }
    }

    if (islan) {
        if (mac_str_to_bin(mac_str, mac_bin) != 0) {
            printf("mac:%s is err!\n", optarg);
            return -1;
        }

        for (i = 0; i < 6 ; i++) {
            puc_data[i] = 0xff;
        }

        for (i = 0; i < 16; i++) {        
            memcpy(&puc_data[i*6 + 6], mac_bin, 6);
        }
        i_ret = send_udp(INADDR_BROADCAST, 1, puc_data, 102);      
        if (i_ret == 0) {
            printf("send to %s success!\n", mac_str);
        }
    }

    
    //test_usage();

}
