

## zookeeper API

```c
typedef void (*watcher_fn)(zhandle_t *zh, int type,
        int state, const char *path,void *watcherCtx);
```

这是zookeeper的事件回调函数，有两种回调方式legacy 以及 watcher object。其实就是一个带参数，一个不带参数，在不明白为什么这么起名。
legacy：用于zookeeper_init() 这类函数
watcher object:用于带w前缀的函数如zoo_wget() 可以传入参数

zookeeper_init() 异步
zoo_wget()       同步
