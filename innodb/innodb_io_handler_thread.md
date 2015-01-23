


## innodb io_handler_thread 

>DECLARE_THREAD(io_handler_thread)(void* arg)

参数arg是aio array中的segment值。一个segment包含4个extend,一个extend包含64个page，page中存放着各种数据，一个page 16KB。segment的信息详细在os/os0file.cc文件中进行封装

### slots以及array--两个重要的aio数据结构

static const ulint IO_IBUF_SEGMENT = 0;
static const ulint IO_LOG_SEGMENT = 1; 

上面分别为insert buffer以及log的Segment编号。aio array的数据结构如下

```cpp
/** The asynchronous i/o array slot structure */
struct os_aio_slot_t{
  ibool   is_read;  /*!< TRUE if a read operation */
  ulint   pos;    /*!< index of the slot in the aio
          array */
  ibool   reserved; /*!< TRUE if this slot is reserved */
  time_t    reservation_time;/*!< time when reserved */
  ulint   len;    /*!< length of the block to read or
          write */
  byte*   buf;    /*!< buffer used in i/o */
  ulint   type;   /*!< OS_FILE_READ or OS_FILE_WRITE */
  os_offset_t offset;   /*!< file offset in bytes */
  os_file_t file;   /*!< file where to read or write */
  const char* name;   /*!< file name or path */
  ibool   io_already_done;/*!< used only in simulated aio:
          TRUE if the physical i/o already
          made and only the slot message
          needs to be passed to the caller
          of os_aio_simulated_handle */
  fil_node_t* message1; /*!< message which is given by the */
  void*   message2; /*!< the requester of an aio operation
          and which can be used to identify
          which pending aio operation was
          completed */
  struct iocb control;  /* Linux control block for aio */
  int   n_bytes;  /* bytes written/read. */
  int   ret;    /* AIO return code */
};

struct os_aio_array_t{
  os_ib_mutex_t mutex;  /*!< the mutex protecting the aio array */
  os_event_t  not_full;
        /*!< The event which is set to the
        signaled state when there is space in
        the aio outside the ibuf segment */
  os_event_t  is_empty;
        /*!< The event which is set to the
        signaled state when there are no
        pending i/os in this array */
  ulint   n_slots;/*!< Total number of slots in the aio                                                                                               
        array.  This must be divisible by
        n_threads. */
  ulint   n_segments;
        /*!< Number of segments in the aio
        array of pending aio requests. A
        thread can wait separately for any one
        of the segments. */
  ulint   cur_seg;/*!< We reserve IO requests in round
        robin fashion to different segments.
        This points to the segment that is to
        be used to service next IO request. */
  ulint   n_reserved;
        /*!< Number of reserved slots in the
        aio array outside the ibuf segment */
  os_aio_slot_t*  slots;  /*!< Pointer to the slots in the array */
  io_context_t*   aio_ctx;
        /* completion queue for IO. There is
        one such queue per segment. Each thread
        will work on one ctx exclusively. */
  struct io_event*  aio_events;
        /* The array to collect completed IOs.
        There is one such event for each
        possible pending IO. The size of the
        array is equal to n_slots. */
};
```
上面的两个数据结构比较重要，os_aio_slot_t一个slot就是一个具体的IO操作，或读或写，读写字节数目等等信息；一个os_aio_array_t是相同目的的一组IO操作，比如IBUF的IO操作，WRITE操作，READ操作,代码有这样的定义
```cpp
static os_aio_array_t*  os_aio_read_array = NULL; /*!< Reads */                                                                                       
static os_aio_array_t*  os_aio_write_array  = NULL; /*!< Writes */
static os_aio_array_t*  os_aio_ibuf_array = NULL; /*!< Insert buffer */
static os_aio_array_t*  os_aio_log_array  = NULL; /*!< Redo log */
static os_aio_array_t*  os_aio_sync_array = NULL; /*!< Synchronous I/O */

```
在os_aio_array_t中，通过读代码可以看出结构关系是，一个segment对应一个io_ctx,对应n个io_event，对应n个slots。
在函数os_aio_init中进行初始化。以4个io read以及4个io write为例。会创建10个IO线程,每一个segment对应256个slots，即一个线程最多能有256的IO requests
---------
|   0   |--> os_aio_ibuf_array
---------
|   1   |--> os_aio_log_array
---------
|   2   |-->  |                        | io_ctx[0],aio_evetns[0.255],slots[0..255]
---------     |                        |
|   3   |---->|                        | io_ctx[1],aio_events[256..512],slots[256..512]
---------     |                        |
|   4   |---->|-->os_aio_read_array <--| io_ctx[2],aio_events[512..768],slots[512..768]
---------     |                        |
|   5   |-->  |                        | io_ctx[3],aio_events[769..1024],slots[769..1024]
---------
|   6   |-->  |
---------     |
|   7   |---->|-->os_aio_write_array
---------     |
|   8   |---->|
---------     |
|   9   |-->  |
---------
|   10  |--> os_aio_sync_array 100 slots 
--------
上面的数字为IO线程的全局segment号 10号没有对应的线程，是线程间协作用来sync的aio数组。读写array是相似的结构。


### IO事件完成查询
io 线程的任务就是等待读写IO事件然后进行异步的读写操作，使用到了Linux native aio。住线程代码如下

```cpp
DECLARE_THREAD(io_handler_thread)(void* arg)  /*!< in: pointer to the number of the segment in the aio array */
{ 
  ulint segment;
  segment = *((ulint*) arg);                                                                                                                   
  while (srv_shutdown_state != SRV_SHUTDOWN_EXIT_THREADS) {
    fil_aio_wait(segment);
  }
} 
```

下面看一下fil_aio_wait这个函数是干嘛的。fil_aio_wait主要代码如下
```cpp
void
fil_aio_wait(ulint segment)  /*!< in: the number of the segment in the aio
                              array to wait for */
{
  ibool   ret;
  fil_node_t* fil_node;
  void*   message;
  ulint   type;
 
  srv_set_io_thread_op_info(segment, "native aio handle");
  ret = os_aio_linux_handle(segment, &fil_node, &message, &type);
  if (fil_node == NULL) {
    ut_ad(srv_shutdown_state == SRV_SHUTDOWN_EXIT_THREADS);
    return;
  }
  srv_set_io_thread_op_info(segment, "complete io for fil node");
  mutex_enter(&fil_system->mutex);
  fil_node_complete_io(fil_node, fil_system, type);
  mutex_exit(&fil_system->mutex);

  /* Do the i/o handling */
  /* IMPORTANT: since i/o handling for reads will read also the insert
  buffer in tablespace 0, you have to be very careful not to introduce
  deadlocks in the i/o system. We keep tablespace 0 data files always
  open, and use a special i/o thread to serve insert buffer requests. */
 
  if (fil_node->space->purpose == FIL_TABLESPACE) {
    srv_set_io_thread_op_info(segment, "complete io for buf page");
    buf_page_io_complete(static_cast<buf_page_t*>(message));
  } else {
    srv_set_io_thread_op_info(segment, "complete io for log");
    log_io_complete(static_cast<log_group_t*>(message));
  }
}

```

os_aio_linux_handle这个函数处理异步的IO事件，这个函数会一直等待知道有IO事件完成，内部会调用函数io_getevents函数。os_aio_linux_handler会检查对应slot的segment是否有完成的，如果有专到found代码段主要是做一些清理以及slot状态重置操作，否则就会调用os_aio_linux_collect一直等待IO事件的完成。大致代码如下
```cpp
ibool os_aio_linux_handle(
  ulint global_seg, /*!< in: segment number in the aio array
        to wait for; segment 0 is the ibuf
        i/o thread, segment 1 is log i/o thread,
        then follow the non-ibuf read threads,
        and the last are the non-ibuf write
        threads. */
  fil_node_t**message1, /*!< out: the messages passed with the */
  void**  message2, /*!< aio request; note that in case the
        aio operation failed, these output
        parameters are valid and can be used to
        restart the operation. */
  ulint*  type)   /*!< out: OS_FILE_WRITE or ..._READ */
{
  ulint   segment;
  os_aio_array_t* array;
  os_aio_slot_t*  slot;
  ulint   n;
  ulint   i;
  ibool   ret = FALSE;
 
  segment = os_aio_get_array_and_local_segment(&array, global_seg);
  n = array->n_slots / array->n_segments;
  
  /* Loop until we have found a completed request. */
  for (;;) {
    ibool any_reserved = FALSE;
    os_mutex_enter(array->mutex);
    for (i = 0; i < n; ++i) {
      slot = os_aio_array_get_nth_slot(
        array, i + segment * n);
      if (!slot->reserved) {
        continue;
      } else if (slot->io_already_done) {
        /* Something for us to work on. */
        goto found;
      } else {
        any_reserved = TRUE;
      }
    }
  
    os_mutex_exit(array->mutex);
  
    /* There is no completed request.
    If there is no pending request at all,
    and the system is being shut down, exit. */     
    if (UNIV_UNLIKELY
        (!any_reserved
         && srv_shutdown_state == SRV_SHUTDOWN_EXIT_THREADS)) {
      *message1 = NULL;
      *message2 = NULL;
      return(TRUE);
    }
  
    /* Wait for some request. Note that we return
    from wait iff we have found a request. */
  
    srv_set_io_thread_op_info(global_seg, "waiting for completed aio requests");
    os_aio_linux_collect(array, segment, n);
  }
  
found:
  /* Note that it may be that there are more then one completed
  IO requests. We process them one at a time. We may have a case
  here to improve the performance slightly by dealing with all
  requests in one sweep. */
  srv_set_io_thread_op_info(global_seg,
        "processing completed aio requests");
  
  *message1 = slot->message1;
  *message2 = slot->message2;
  
  *type = slot->type;
  
  if (slot->ret == 0 && slot->n_bytes == (long) slot->len) {
  
    ret = TRUE;
  } else {
    errno = -slot->ret;
  
    /* os_file_handle_error does tell us if we should retry
    this IO. As it stands now, we don't do this retry when
    reaping requests from a different context than
    the dispatcher. This non-retry logic is the same for
    windows and linux native AIO.
    We should probably look into this to transparently                                                                                                
    re-submit the IO. */
    os_file_handle_error(slot->name, "Linux aio");
 
    ret = FALSE;
  }
 
  os_mutex_exit(array->mutex);
 
  os_aio_array_free_slot(array, slot);
 
  return(ret);
}

```

### IO事件的生产

I/O请求在os_aio_linux_dispatch中进行提交。这个函数就是对io_submit的一个封装，很简单。不赘述。

### IO事件完成后的io_handle的工作--fil_node



fil_system_t结构
```cpp
/** The tablespace memory cache; also the totality of logs (the log
data space) is stored here; below we talk about tablespaces, but also
the ib_logfiles form a 'space' and it is handled here */
struct fil_system_t {
  ib_mutex_t    mutex;    /*!< The mutex protecting the cache */
  hash_table_t* spaces;   /*!< The hash table of spaces in the system; they are hashed on the space id */
  hash_table_t* name_hash;  /*!< hash table based on the space name */
  UT_LIST_BASE_NODE_T(fil_node_t) LRU; 
          /*!< base node for the LRU list of the
          most recently used open files with no
          pending i/o's; if we start an i/o on
          the file, we first remove it from this
          list, and return it to the start of
          the list when the i/o ends;
          log files and the system tablespace are
          not put to this list: they are opened
          after the startup, and kept open until
          shutdown */
  UT_LIST_BASE_NODE_T(fil_space_t) unflushed_spaces;
          /*!< base node for the list of those
          tablespaces whose files contain
          unflushed writes; those spaces have
          at least one file node where
          modification_counter > flush_counter */
  ulint   n_open;   /*!< number of files currently open */
  ulint   max_n_open; /*!< n_open is not allowed to exceed this */
  ib_int64_t  modification_counter;/*!< when we write to a file we increment this by one */
  ulint   max_assigned_id;/*!< maximum space id in the existing
          tables, or assigned during the time
          mysqld has been up; at an InnoDB
          startup we scan the data dictionary
          and set here the maximum of the
          space id's of the tables there */
  ib_int64_t  tablespace_version;
          /*!< a counter which is incremented for
          every space object memory creation;
          every space mem object gets a
          'timestamp' from this; in DISCARD/
          IMPORT this is used to check if we
          should ignore an insert buffer merge
          request */
  UT_LIST_BASE_NODE_T(fil_space_t) space_list;
          /*!< list of all file spaces */            
  ibool   space_id_reuse_warned;
          /* !< TRUE if fil_space_create()
          has issued a warning about
          potential space_id reuse */
};         

template <typename TYPE>
struct ut_list_node {
  TYPE*   prev; /*!< pointer to the previous node,
      NULL if start of list */
  TYPE*   next; /*!< pointer to next node, NULL if end of list */
};
 
#define UT_LIST_NODE_T(TYPE)  ut_list_node<TYPE>
```

fil/fil0fil.cc 这个文件是innodb tablespace and log cache的实现，这里有三个重要的数据结构:fil_system_t这是全局的文件系统包含所有的space，fil_space_t这就一个tablespace或者一个log,；fil_node_t这是一个具体的IO结构，每一个fil_node_t中存储着一个文件描述符
