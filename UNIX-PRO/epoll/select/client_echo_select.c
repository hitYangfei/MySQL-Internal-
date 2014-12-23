#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define MAXLINE     1024
#define IPADDRESS   "127.0.0.1"
#define SERV_PORT   8787

#define max(a,b) (a > b) ? a : b

static void handle_connection(int sockfd);

int main(int argc,char *argv[])
{
    int                 sockfd;
    struct sockaddr_in  servaddr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);
    connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    //处理连接描述符
    handle_connection(sockfd);
    return 0;
}

static void handle_connection(int sockfd)
{
    char    sendline[MAXLINE],recvline[MAXLINE];
    int     maxfdp,stdineof;
    fd_set  rset;
    int n;
    FD_ZERO(&rset);
    for (; ;)
    {
        //添加标准输入描述符
        FD_SET(STDIN_FILENO,&rset);
        //添加连接描述符
        FD_SET(sockfd,&rset);
        maxfdp = max(STDIN_FILENO,sockfd);
        //进行轮询
        select(maxfdp+1,&rset,NULL,NULL,NULL);
        //测试连接套接字是否准备好
        if (FD_ISSET(sockfd,&rset))
        {
            n = read(sockfd,recvline,MAXLINE);
            if (n == 0)
            {
                    fprintf(stderr,"client: server is closed.\n");
                    close(sockfd);
                    FD_CLR(sockfd,&rset);
            }
            write(STDOUT_FILENO,recvline,n);
        }
        //测试标准输入是否准备好
        if (FD_ISSET(STDIN_FILENO,&rset))
        {
            n = read(STDIN_FILENO,sendline,MAXLINE);
            if (n  == 0)
            {
                FD_CLR(STDIN_FILENO,&rset);
                continue;
            }
            write(sockfd,sendline,n);
        }
    }
}
