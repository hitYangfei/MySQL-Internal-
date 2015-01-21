#define _GNU_SOURCE   /* syscall() is not POSIX */

#include <stdio.h>    /* for perror() */
#include <unistd.h>   /* for syscall() */
#include <sys/syscall.h>  /* for __NR_* definitions */
#include <linux/aio_abi.h>  /* for AIO types and constants */
#include <fcntl.h>    /* O_RDWR */
#include <string.h>   /* memset() */
#include <inttypes.h> /* uint64_t */

/*inline int io_setup(unsigned nr, aio_context_t *ctxp)
{
  return syscall(__NR_io_setup, nr, ctxp);
}

inline int io_destroy(aio_context_t ctx) 
{
  return syscall(__NR_io_destroy, ctx);
}

inline int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp) 
{
  return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
    struct io_event *events, struct timespec *timeout)
{
  return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}
*/
int main()
{
  aio_context_t ctx;
  struct iocb cb;
  struct iocb *cbs[1];
  char data[4096];
  struct io_event events[1];
  int ret;
  int fd;

  fd = open("/tmp/testfile", O_RDWR | O_CREAT);
  if (fd < 0) {
    perror("open error");
    return -1;
  }

  ctx = 0;

  ret = io_setup(128, &ctx);
  if (ret < 0) {
    perror("io_setup error");
    return -1;
  }

  /* setup I/O control block */
  memset(&cb, 0, sizeof(cb));
  cb.aio_fildes = fd;
  cb.aio_lio_opcode = IOCB_CMD_PWRITE;

  memset(data, 'a', 4096);
  data[4096]=0;
  /* command-specific options */
  cb.aio_buf = (uint64_t)data;
  cb.aio_offset = 0;
  cb.aio_nbytes = 4096;

  cbs[0] = &cb;

  ret = io_submit(ctx, 1, cbs);
  if (ret != 1) {
    if (ret < 0)
        perror("io_submit error");
    else
      fprintf(stderr, "could not sumbit IOs");
    return  -1;
  }

  /* get the reply */
  ret = io_getevents(ctx, 1, 1, events, NULL);
  printf("getevent ret %d\n", ret);

  ret = io_destroy(ctx);
  if (ret < 0) {
    perror("io_destroy error");
  return -1;
  }

  return 0;
}

