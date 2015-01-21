## linux native aio

Linux aio API有两种，分别为linux native aio api(libaio)以及POSIX AIO API(GLIBC) Innodb使用的是前者，本地的aio而不是glibc的aio

linux native aio api只有五个系统调用

```c
#include <linux/aio_abi.h>

int io_setup(unsigned nr_events, aio_context_t *ctxp);

int io_destroy(aio_context_t ctx);

int io_submit(aio_context_t ctx_id, long nr, struct iocb **iocbpp);

int io_cancel(aio_context_t ctx, struct iocb *, struct io_event *result);

int io_getevents(aio_context_t ctx, long min_nr, long nr,
      struct io_event *events, struct timespec *timeout);
```

> io_setup() 这个函数创建一个异步IO的环境变量aio_context_t，在调用这个函数之前ctxp这个变量应该为0,nr_events为事件的个数，如果成功返回0
> io_submi() 这个函数会提交读写IO申请，nr个，iocbpp是一个nr长度的aio 控制块。
> io_getevents() 等待事件完成，最少min_nr个最多nr个，超时timeout


这些系统调用都需要使用syscall进行调用

比如1.c 直接gcc编译即可，使用了syscall来进行系统调用

而2.c则没有使用syscall进行封装，编译的时候需要gcc 2.c -laio。
