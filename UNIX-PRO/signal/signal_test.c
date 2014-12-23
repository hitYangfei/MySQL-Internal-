#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

unsigned int is_abort = 0;


void sig_handler(int signum) {
  switch (signum) {
  case SIGHUP:
    printf("sighup\n");
    break;
  /*
   * CTRL + C will send SIGINT signal
   */
  case SIGINT:
    printf("SIGINT\n");
    is_abort = 1;
    break;
  case SIGTERM:
    printf("SIGTERM\n");
    break;
  defaults:
    printf("%d\n", signum);
  }
}

int main() {
  signal(SIGHUP, sig_handler);
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
  while (!is_abort) ;
  return 0;
}
