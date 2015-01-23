

## PSI

读代码的过程中看到各种PSI-XX开头的变量，PSI代表：Performance schame instrumentation

### innodb 架构

some threads <====> some buffer <====> some disk io

threads: show engine innodb status


FILE I/O:

I/O thread 0 state: waiting for completed aio requests (insert buffer thread)
I/O thread 1 state: waiting for completed aio requests (log thread)
I/O thread 2 state: waiting for completed aio requests (read thread)
I/O thread 3 state: waiting for completed aio requests (read thread)
I/O thread 4 state: waiting for completed aio requests (read thread)
I/O thread 5 state: waiting for completed aio requests (read thread)
I/O thread 6 state: waiting for completed aio requests (write thread)
I/O thread 7 state: waiting for completed aio requests (write thread)
I/O thread 8 state: waiting for completed aio requests (write thread)
I/O thread 9 state: waiting for completed aio requests (write thread)

除此之外还有man thread, lock监控线程以及错误监控线程。


some buffer:

innodb_buffer_pool_size
innodb_log_buffer_size
innodb_additional_mem_pool_size


----------------------------------------------------------------------------
|          innodb 内存架构图                                               |
|                                                                          |
---------------------------------- -----------------------------------------
|                                | |    缓冲池innodb_buffer_pool           |
|     日志缓冲(log buffer)       | |                                       |
|                                | | 1. 数据页                             |
---------------------------------- | 2. 插入缓冲                           |
|                                  | 3. 锁信息                             |
---------------------------------  | 4. 索引页                             |
|                                | | 5. 自适应hash                         |
| 额外内存池                     | | 6. 数据字典信息                       |
| innodb_additional_mem_pool_size| |                                       |
|                                | |                                       |
----------------------------------------------------------------------------



### innodb threads

```cpp
static PSI_thread_info  all_innodb_threads[] = {
  {&trx_rollback_clean_thread_key, "trx_rollback_clean_thread", 0},
  {&io_handler_thread_key, "io_handler_thread", 0},
  {&srv_lock_timeout_thread_key, "srv_lock_timeout_thread", 0},
  {&srv_error_monitor_thread_key, "srv_error_monitor_thread", 0},
  {&srv_monitor_thread_key, "srv_monitor_thread", 0},
  {&srv_master_thread_key, "srv_master_thread", 0},
  {&srv_purge_thread_key, "srv_purge_thread", 0},
  {&buf_page_cleaner_thread_key, "page_cleaner_thread", 0},
  {&recv_writer_thread_key, "recv_writer_thread", 0}
};
```

调用innodb_init()-->inline_mysql_thread_register()
定义在ha_innodb.cc中，可以看出这些线程就是INNODB的所有工作线程，搞明白每一个线程是干什么的就应该掌握INNODB内核了。


### main thread：src_master_thread

10 file io thread     str0start.cc:io_handler_thread (linux aio)

src_master_thread

  loop:
  这里有一个sleep函数为os_thread_sleep() 这个函数的实现比较有意思竟然是用 select实现的 select(0, NULL, NULL, NULL, &t); 就这样一行代码，挺有意思。不明白为什么要用select实现感觉有点小题大做。 这个loop每1s执行一次。
  suspend_thread:
  挂起线程


```cpp
loop:
  // innodb 的可调参数 innodb_force_recovery
  if (srv_force_recovery >= SRV_FORCE_NO_BACKGROUND) {
    goto suspend_thread;
  }
  while (srv_shutdown_state == SRV_SHUTDOWN_NONE) {
    srv_master_sleep();   // sleep 1 s
    MONITOR_INC(MONITOR_MASTER_THREAD_SLEEP);
    if (srv_check_activity(old_activity_count)) {
      old_activity_count = srv_get_activity_count();
      srv_master_do_active_tasks();
    } else {
      srv_master_do_idle_tasks();
    }
  }
  while (srv_master_do_shutdown_tasks(&last_print_time)) {
    /* Shouldn't loop here in case of very fast shutdown */
    ut_ad(srv_fast_shutdown < 2);
  }
suspend_thread:
  srv_main_thread_op_info = "suspending";
  srv_suspend_thread(slot);
  srv_main_thread_op_info = "waiting for server activity";
  os_event_wait(slot->event);
  if (srv_shutdown_state == SRV_SHUTDOWN_EXIT_THREADS) {
    os_thread_exit(NULL);
  }
  goto loop;

```

slot的声明为rv_slot_t* slot，innodb内部保持着一个thread table，每一个table元素就是一个slot，这里的slot指的就是当前线程的线程信息。

是可以看到在loop的过程中，有一个while循环每1s一次，要么是activity执行srv_master_do_active_tasks要么没有新任务执行srv_master_do_idle_tasks,走哪一个分支由函数srv_check_activity()决定

`cpp
UNIV_INTERN
ibool
srv_check_activity(ulint old_activity_count) /*!< in: old activity count */
{
  return(srv_sys->activity_count != old_activity_count);
}
```

这个检查函数是比较之前的activity_count与现在的activity_count是否一致，即可以简单的理解为1s之前和现在的进行比较。srv_sys->activity_count的值通过函数srv_inc_activity_count(void)进行自增操作，只通过这个函数进行修改。代码中的active还是idle状态是针对mysql server而言的，有query request就是activice否则就是idle。

#### srv_master_do_idel_tasks

```cpp
static
void
srv_master_do_idle_tasks(void)
/*==========================*/
{
  ++srv_main_idle_loops;

  /* ALTER TABLE in MySQL requires on Unix that the table handler
  can drop tables lazily after there no longer are SELECT
  queries to them. */
  srv_main_thread_op_info = "doing background drop tables";
  row_drop_tables_for_mysql_in_background();
  if (srv_shutdown_state > 0) {
    return;
  }
  /* make sure that there is enough reusable space in the redo
  log files */
  srv_main_thread_op_info = "checking free log space";
  log_free_check();

  /* Do an ibuf merge */
  srv_main_thread_op_info = "doing insert buffer merge";
  ibuf_contract_in_background(0, TRUE);
  if (srv_shutdown_state > 0) {
    return;
  }
  srv_main_thread_op_info = "enforcing dict cache limit";
  srv_master_evict_from_table_cache(100);
  /* Flush logs if needed */
  srv_sync_log_buffer_in_background();
  if (srv_shutdown_state > 0) {
    return;
  /* Make a new checkpoint */
  srv_main_thread_op_info = "making checkpoint";
  log_checkpoint(TRUE, FALSE);
}
```

从注释中可以看到这个idel task是做什么的。这个函数会做6件事
1. drop table 
2. check log space 
3. doing insert buffer merge
4. enforcing dict cache limit
5. flush logs if needed
6. make a new check point

#### srv_master_do_active_tasks

这个active的过程比idle过程多了一些人物，这个函数做如下事情
1. drop table
2. check log space
3. doing insert buffer merge
4. flush logs if needed
5. 每SRV_MASTER_MEM_VALIDATE_INTERVAL秒做一次内存检测
6. 每SRV_MASTER_DICT_LRU_INTERVAL秒做一次enforcing dict cache limit
7. 每SRV_MASTER_CHECKPOINT_INTERVAL秒做一次Make a new checkpoint

这些宏定义如下:
```cpp
# define  SRV_MASTER_CHECKPOINT_INTERVAL    (7)
# define  SRV_MASTER_PURGE_INTERVAL   (10)
#ifdef MEM_PERIODIC_CHECK
# define  SRV_MASTER_MEM_VALIDATE_INTERVAL  (13)
#endif /* MEM_PERIODIC_CHECK */
# define  SRV_MASTER_DICT_LRU_INTERVAL    (47)

```
为什么定义为这样的数，这四个数是互质的，这样做的目的是为了平衡负载。SRV_MASTER_PURGE_INTERVAL这个宏没有在任何定义看到被使用，应该是之前用过的宏被弃用了，还有待于考证。源码注释如下
```cpp
/* Interval in seconds at which various tasks are performed by the
master thread when server is active. In order to balance the workload,
we should try to keep intervals such that they are not multiple of
each other. For example, if we have intervals for various tasks
defined as 5, 10, 15, 60 then all tasks will be performed when
current_time % 60 == 0 and no tasks will be performed when
current_time % 5 != 0. */
```


