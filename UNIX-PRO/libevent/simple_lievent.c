#include<stdio.h>
#include<stdlib.h>
#include<event.h>
#include<signal.h>

#include <string.h>
#include <errno.h>
 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS   "127.0.0.1"
#define PORT        8787

static int socket_bind(const char* ip,int port);
struct event ev;
struct timeval tv, tv1;
void time_cb(int fd, short event, void *argc)
{
  printf("timer wakeup\n");
  event_add(&ev, &tv); // reschedule timer
}
void sig_handler(int fd, short event, void *argv)
{
  printf("reveice SIGINT signal\n");
  exit(0);
}
void on_accept(int sock, short event, void* arg)
{
  struct sockaddr_in cli_addr;
  int newfd, sin_size;
  // read_ev must allocate from heap memory, otherwise the program would crash from segmant fault
  sin_size = sizeof(struct sockaddr_in);
  newfd = accept(sock, (struct sockaddr*)&cli_addr, &sin_size);
  fprintf(stdout,"accept a new client: %s:%d\n", inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
} 
int main()
{
  struct event_base *base = event_init();
  int listenfd;
  struct event listen_ev;
  tv.tv_sec = 3; // 10s period
  tv.tv_usec = 0;
  evtimer_set(&ev, time_cb, NULL);
  event_add(&ev, &tv);

  listenfd = socket_bind("127.0.0.1", 13001);
  listen(listenfd, 1024);
  event_set(&listen_ev, listenfd, EV_READ|EV_PERSIST, on_accept, NULL);
  event_base_set(base, &listen_ev);
  event_add(&listen_ev,NULL);
  event_base_dispatch(base);
  return 0;
}
int socket_bind(const char* ip,int port)
{
  int  listenfd;
  struct sockaddr_in servaddr;
  listenfd = socket(AF_INET,SOCK_STREAM,0);
  if (listenfd == -1)
  {
      perror("socket error:");
      exit(1);
  }
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  inet_pton(AF_INET,ip,&servaddr.sin_addr);
  servaddr.sin_port = htons(port);
  if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
  {
      perror("bind error: ");
      exit(1);
  }
  return listenfd;
}

