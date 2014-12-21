
# MySQL源码-----主线程

## per connection per thread

MySQL 采用一个连接一个线程的结构体系。

MySQL 的入口函数为 mysqld_main 函数，这个函数会调用 handle_connections_sockets()函数，在这个函数内部就是对具体连接的处理。这个函数的主要处理代码如下

```cpp
 while (!abort_loop)
  {
#ifdef HAVE_POLL
    retval= poll(fds, socket_count, -1);
#else
    readFDs=clientFDs;

    retval= select((int) max_used_connection,&readFDs,0,0,0);
#endif
   if (abort_loop)
    {
      MAYBE_BROKEN_SYSCALL;
      break;
    }
    ...
    ...
    for (uint retry=0; retry < MAX_ACCEPT_RETRY; retry++)
    {
      size_socket length= sizeof(struct sockaddr_storage);
      new_sock= mysql_socket_accept(key_socket_client_connection, sock,
                                    (struct sockaddr *)(&cAddr), &length);
      ...
      ...
    }
    ...
    ...
    if (!(thd= new THD))
    {
      ...
      ...
    }
    ...
    ...
    create_new_thread(thd);
```

可以从代码中看到 handle_connections_sockets() 使用 select 或者 poll 进行 IO 多路复用，这里的 socket 集合为 TCP/IP 的监听，以及 unix-socket 监听，前者就是使用*mysql -uroot -h127.0.0.1 -P3306*登陆的处理，后者是使用*mysql -uroot --scoket=/tmp/mysqld.socket*的处理。

MySQL 的主函数会一直阻塞在 select() 函数上直到有新的连接到来，当有新的连接到来的时候，首先进行 accept 操作，MySQL 服务对于任意的一个客户端 connect() 都会回发一个 hand shake 数据包，以便下面进行连接使用，更详细的可以参考 MySQL Internal 文档；接下来会创建一个 THD 结构体，然后调用create_new_thread() 函数进行对这个连接线程的分配。

## create_new_thread 为连接分配线程

源码注释已经将这个函数的作用写的很清晰了，这个函数会为连入的客户端连接进行线程的创建，但是如果有处于 idle 状态的的线程则会进行复用，具体由 thread_cache_size 配置选项有关，下面会介绍到。主要代码如下

```cpp
static void create_new_thread(THD *thd)
{
  mysql_mutex_lock(&LOCK_connection_count);
  if (connection_count >= max_connections + 1 || abort_loop)
  {
    close_connection(thd, ER_CON_COUNT_ERROR);
    delete thd;
    ...
    ...
  }
  ++connection_count;

  if (connection_count > max_used_connections)
    max_used_connections= connection_count;
  mysql_mutex_unlock(&LOCK_connection_count);
  mysql_mutex_lock(&LOCK_thread_count);
  thd->thread_id= thd->variables.pseudo_thread_id= thread_id++;

  MYSQL_CALLBACK(thread_scheduler, add_connection, (thd));

}
```

这个函数的作用是其实就是为线程分配一个 thread_id 这个线程ID会在之后的连接握手过程中传送给客户端，同时在这个函数中会修改全局变量 connection_count 以及 thread_id 所以会涉及到两个锁操作: *LOCK_connection_count 以及 LOCK_connection_count*。后者虽在thread_scheduler->add_connection函数中进行释放，这是一个函数指针，具体的是调用函数 create_thread_to_handle_connection() 来完成线程的创建或者复用以前还没有释放的线程。下面分别对数据结构 thread_scheduler 以及 add_connection 函数进行说明。

### thread_scheduler

```cpp
struct scheduler_functions
{
  uint max_threads;
  bool (*init)(void);
  bool (*init_new_connection_thread)(void);
  void (*add_connection)(THD *thd);
  void (*thd_wait_begin)(THD *thd, int wait_type);
  void (*thd_wait_end)(THD *thd);
  void (*post_kill_notification)(THD *thd);
  bool (*end_thread)(THD *thd, bool cache_thread);
  void (*end)(void);
};
```

thread_scheduler 数据结构比较直观就是一些函数指针，这些函数都是用来管理 MySQL 线程的，辅助实现线程池的实现，但是其实 MySQL 并没有真正意思的线程池，不妨称为一个伪线程池，下面会进行介绍。

### create_thread_to_handle_connection 具体分配线程处理连接函数

```cpp
static std::list<THD*> *waiting_thd_list= NULL;

void create_thread_to_handle_connection(THD *thd)
{
  if (blocked_pthread_count >  wake_pthread)
  {
    waiting_thd_list->push_back(thd);
    wake_pthread++;
    mysql_cond_signal(&COND_thread_cache);
  }
  else
  {
  ...
  ...
    if ((error= mysql_thread_create(key_thread_one_connection,
                                    &thd->real_id, &connection_attrib,
                                    handle_one_connection,
                                    (void*) thd)))
    {
    ...
    ...
    }
    add_global_thread(thd);
  }
  mysql_mutex_unlock(&LOCK_thread_count);

}
```

waiting_thd_list 的声明在第一行， MySQL 内部有各种不同链表的实现，可能是由于每一人的代码风格不同吧，所以使用的链表实现方式也不用。

在第一个判断分支中进行 thread 复用,如果进入if条件分支，则不必进行创建新的线程否则进入else分支进行线程的重新创建。在复用的if代码中只是唤醒阻塞在COND_thread_cache条件变量的线程，所以可以猜到之前的连接退出后，如果满足线程复用的条件，则不会进行销毁会阻塞到COND_thread_cache条件变量上，然后等待复用。

如果无法复用，则会创建新的线程使用handle_one_connection函数进行处理，这个函数会调用do_handle_one_connection函数进行具体某一个连接的处理，主要处理函数为do_command以处理各种客户端发来的请求。下面看一个do_handle_one_connection函数的实现。

```cpp
void do_handle_one_connection(THD *thd_arg)
{
  ...
  ...
  for (;;)
  {
    bool rc;

    NET *net= &thd->net;
    mysql_socket_set_thread_owner(net->vio->mysql_socket);

    rc= thd_prepare_connection(thd);
    if (rc)
      goto end_thread;

    while (thd_is_connection_alive(thd))
    {
      mysql_audit_release(thd);
      if (do_command(thd))
        break;
    }
    end_connection(thd);
end_thread:
    close_connection(thd);
    if (MYSQL_CALLBACK_ELSE(thread_scheduler, end_thread, (thd, 1), 0))
      return;                                 // Probably no-threads

    /*
      If end_thread() returns, we are either running with
      thread-handler=no-threads or this thread has been schedule to
      handle the next connection.
    */
    thd= current_thd;
    thd->thread_stack= (char*) &thd;
  }
}

```

正常如果客户端的连接一直进行数据请求则会一直在while循环中进行循环，使用do_command函数进行处理，但是如果这是这种逻辑那么一重while循环就足够了就没必要外层的for循环了，外层for循环就是进行线程复用的。通过上面的数据结构thread_scheduler的end_thread函数实现，具体通过block_until_new_connection() 函数进行实现的。

## MySQL 线程池，thread_cache_size配置选项------block_until_new_connection

```cpp
static bool block_until_new_connection()
{
  mysql_mutex_lock(&LOCK_thread_count);
  if (blocked_pthread_count < max_blocked_pthreads &&
      !abort_loop && !kill_blocked_pthreads_flag)
  {
    blocked_pthread_count++;
    while (!abort_loop && !wake_pthread && !kill_blocked_pthreads_flag)
      mysql_cond_wait(&COND_thread_cache, &LOCK_thread_count);

    blocked_pthread_count--;
    if (wake_pthread)
    {
      THD *thd;
      wake_pthread--;
      waiting_thd_list->pop_front();
      thd->thread_stack= (char*) &thd;          // For store_globals
      (void) thd->store_globals();
      add_global_thread(thd);
      mysql_mutex_unlock(&LOCK_thread_count);
      return true;
    }
  }
  mysql_mutex_unlock(&LOCK_thread_count);
  return false;
}

```

在一个客户端连接quit后会调用这个函数，可以看到当满足if条件时会进行线程复用，当满足if条件后，会一直阻塞在mysql_cond_wait中，等待两个变量，在上面的create_thread_to_handle_connection函数已经说明过对于这两个条件变量的处理。而*max_blocked_pthreads*变量就是配置选项*thread_cache_size*配置的值。

当可以复用时，会到waiting_thd_list队列中从头取出一个线程进行复用。

## 总结

其实纵观这部分代码还是比较清晰的，但是作者能力有限可能说的不太清晰，如果想要了解的更加详细具体还是建议去读一读，所有的代码都来自 mysql5.6.22 版本。在 mysql_main 函数中还有很多的初始化以及其他的工作，本文并没有进行说明，甚至连socket是如何初始化的也没有进行说明，本文的重点其实重要是 MySQL 的 *per connection per thread* 结构的实现以及一个简单的伪线程池的实现。
