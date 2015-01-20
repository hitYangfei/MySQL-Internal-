


## innodb io_handler_thread 

>DECLARE_THREAD(io_handler_thread)(void* arg)

参数arg是aio array中的segment值。一个segment包含4个extend,一个extend包含64个page，page中存放着各种数据，一个page 16KB。segment的信息详细在os/os0file.cc文件中进行封装

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

os_aio_linux_handle这个函数处理异步的IO事件，这个函数会一直等待知道有IO事件完成，内部会调用函数io_getevents函数。
