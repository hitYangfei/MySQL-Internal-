## libevent 

### libevent 事件注册 --- event_io_map

```c
#define event_io_map event_signal_map

/* Used to map signal numbers to a list of events.  If EVMAP_USE_HT is not
   defined, this structure is also used as event_io_map, which maps fds to a
   list of events.
*/
struct event_signal_map {
  /* An array of evmap_io * or of evmap_signal *; empty entries are
   * set to NULL. */
  void **entries;
  /* The number of entries available in entries */
  int nentries;
};

```

这个就是一个简单的数组表示的hash table ，他的slot就是一个文件描述符fd的值。

对于event_io_map来说hash table的入口类型为

```c
struct evmap_io {
  struct event_list events;
  ev_uint16_t nread;
  ev_uint16_t nwrite;
};

#define TAILQ_HEAD(name, type)      \
struct name {         \
  struct type *tqh_first;     \
  struct type **tqh_last;     \
}

TAILQ_HEAD (event_list, event);
```

可以看到入口结构是一个链表，一个event的双链表，所以event_io_map使用链表来解决hash冲突问题。

evmap_io_add函数的重要部分被宏定义所掩盖，使用gcc预编译干掉宏定义的干扰看一下它的实现
```c
do {
  if (io->entries[fd] == (void *)0) {
    (io)->entries[fd] = event_mm_calloc_((1), (sizeof(struct evmap_io)+evsel->fdinfo_len));
    if (__builtin_expect(!!((io)->entries[fd] == ((void *)0)),0))
      return (-1);
    (evmap_io_init)((struct evmap_io *)(io)->entries[fd]);
  }
  (ctx) = (struct evmap_io *)((io)->entries[fd]);
} while (0) ;
```

libevent的evnet_io_map是一个动态可扩展的hash table,如果空间不足使用realloc在原来的基础上进行内存申请。

综上event_io_map是一个存储key为fd，value为event的hash table，对于每一个调用event_add()的调用，都会将对应的fd以及event插入到这个event_io_map这个hash table中以便后面进行使用。其实就是一个事件注册的过程。

### libevent主循环

libevent通过一组接口屏蔽等层的poll epoll select等系统调用从而实现跨平台。这个接口为eventtop
```c
struct eventop {
  const char *name;
  void *(*init)(struct event_base *); 
  int (*add)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
  int (*del)(struct event_base *, evutil_socket_t fd, short old, short events, void *fdinfo);
  int (*dispatch)(struct event_base *, struct timeval *); 
  void (*dealloc)(struct event_base *); 
  int need_reinit;
  enum event_method_feature features;
  size_t fdinfo_len;
};

```

注册完事件口就可以进入主循环了，通过函数event_base_loop实现，这个函数会调用eventop中的dispatch进行阻塞等待直到超时或者有事件可用

>res = evsel->dispatch(base, tv_p);

以epool为例进行这个函数的具体说明

```c
static int epoll_dispatch(struct event_base *base, struct timeval *tv)
{
  ...
  res = epoll_wait(epollop->epfd, events, epollop->nevents, timeout);
  ...
   for (i = 0; i < res; i++) {
    ...
    evmap_io_active(base, events[i].data.fd, ev | EV_ET);
  }
  ...
}
```

epoll_dispatch函数会调用epoll_wait进行阻塞等待，然后将准备好的io进行evmap_io_active激活，evmap_io_active的过程如下

1. 通过event_io_map得到此fd的所有event，即一个event的链表
2. 遍历step 1中的event链表将其插入base-&gt;activequeues[ev-&gt;ev_pri]中
3. 如果是一个超时的则会有一个最小堆来维护

下面看一下主循环代码
```c
int event_base_loop(struct event_base *base, int flags)
{
  ...
  while (!done)
  {
    ...
    res = evsel->dispatch(base, tv_p);
    ...
    timeout_process(base);
    if (N_ACTIVE_CALLBACKS(base)) {
      int n = event_process_active(base);
      ..
    }
  }
}
```

在event_process_active中会对epoll_dispatch中所有插入到base的activequeues中的事件进行调用回调函数进行处理。在处理的过程中，activequeues也会被重置。
