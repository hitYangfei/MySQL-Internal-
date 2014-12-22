## MySQL 内存分配------my_alloc

### 重要的数据结构 USED_MEM 以及 MEM_ROOT

MySQL 的内存管理主要在my_alloc.h 以及 my_alloc.c文件中进行声明和实现。重要的数据结构为 USED_MEM 以及 MEM_ROOT 。定义如下

```cpp
typedef struct st_used_mem
{          /* struct for once_alloc (block) */
  struct st_used_mem *next;    /* Next block in use */
  unsigned int  left;      /* memory left in block  */
  unsigned int  size;      /* size of block */
} USED_MEM;
 
 
typedef struct st_mem_root
{
  USED_MEM *free;                  /* blocks with free memory in it */
  USED_MEM *used;                  /* blocks almost without free memory */                                                                     
  USED_MEM *pre_alloc;             /* preallocated block */
  /* if block have less memory it will be put in 'used' list */
  size_t min_malloc;
  size_t block_size;               /* initial block size */
  unsigned int block_num;          /* allocated blocks counter */
  /*  
     first free block in queue test counter (if it exceed 
     MAX_BLOCK_USAGE_BEFORE_DROP block will be dropped in 'used' list)
  */
  unsigned int first_block_usage;
 
  void (*error_handler)(void);
} MEM_ROOT;

```

数据结构 USED_MEM 用来记录具体分配的内存情况可以理解为一个block，next指针放在结构的第一个位置与 base_list 的二维指针的妙用是一样的，left是当前这个内存block中还剩下的可以使用的内存大小，size为当前分配的block的大小；MEM_ROOT 用来进行内存管理，used链表表示那些block的 *left-= length < min_malloc* 的内存block，即没有剩余空间可以继续给下面的内存申请进行使用，而free链表则恰恰表示还可以继续使用的block内存块，即如果以后申请的内存大小比free链表中的某一个block的left要小则可以对那个block进行复用，直到不可以再复用为止，把block从free链表中移动至used链表即可。所以在使用alloc的内存管理函数时，可以不用在意单个内存的申请与释放，想要内存就进行申请，等某一个阶段结束后一起进行内存释放。

### 重要的内存管理API

```cpp
1. void init_alloc_root(MEM_ROOT *mem_root, size_t block_size,
         size_t pre_alloc_size __attribute__((unused)))
2. void *alloc_root(MEM_ROOT *mem_root, size_t length)
3. void free_root(MEM_ROOT *root, myf MyFlags)
```

这三个重要的 API 分别用来进行内存管理初始化，分配以及释放。

#### init_alloc_root

```cpp
/*
  Initialize memory root
 
  SYNOPSIS
    init_alloc_root()
      mem_root       - memory root to initialize
      block_size     - size of chunks (blocks) used for memory allocation
                       (It is external size of chunk i.e. it should include
                        memory required for internal structures, thus it
                        should be no less than ALLOC_ROOT_MIN_BLOCK_SIZE)
      pre_alloc_size - if non-0, then size of block that should be
                       pre-allocated during memory root initialization.
 
  DESCRIPTION
    This function prepares memory root for further use, sets initial size of
    chunk for memory allocation and pre-allocates first block if specified.
    Altough error can happen during execution of this function if
    pre_alloc_size is non-0 it won't be reported. Instead it will be
    reported as error in first alloc_root() on this memory root.
*/
void init_alloc_root(MEM_ROOT *mem_root, size_t block_size,
         size_t pre_alloc_size __attribute__((unused)))
{
  mem_root->free= mem_root->used= mem_root->pre_alloc= 0;
  mem_root->min_malloc= 32;
  mem_root->block_size= block_size - ALLOC_ROOT_MIN_BLOCK_SIZE;
  mem_root->error_handler= 0;
  mem_root->block_num= 4;     /* We shift this with >>2 */
  mem_root->first_block_usage= 0;
  ...
  ...
}
```

从函数的注释以及代码可以看到会进行块的初始化。 ALLOC_ROOT_MIN_BLOCK_SIZE 是一个最小块大小的宏定义。与具体的机器有关，64位机器为32；block_num是当前这个内存管理的块个数，初始化为4,但是使用的时候会对其进行向右移2位所以结果为1。

#### alloc_root

```cpp
void *alloc_root(MEM_ROOT *mem_root, size_t length)         
{
  uchar* point;               
  reg1 USED_MEM *next= 0;     
  reg2 USED_MEM **prev;   
  length= ALIGN_SIZE(length);   
  if ((*(prev= &mem_root->free)) != NULL)
  {                            
    ...
    ...
    for (next= *prev ; next && next->left < length ; next= next->next)
      prev= &next->next;       
  }                            
  if (! next)                    
  {           /* Time to alloc new block */      
    block_size= mem_root->block_size * (mem_root->block_num >> 2);
    get_size= length+ALIGN_SIZE(sizeof(USED_MEM));
    get_size= MY_MAX(get_size, block_size);
                                 
    if (!(next = (USED_MEM*) my_malloc(get_size,MYF(MY_WME | ME_FATALERROR))))
    {                            
      ...
    }                     
    mem_root->block_num++;                                                                                                                     
    next->next= *prev;    
    next->size= get_size;   
    next->left= get_size-ALIGN_SIZE(sizeof(USED_MEM));
    *prev=next;          
  }
     
  point= (uchar*) ((char*) next+ (next->size-next->left));  
  /*TODO: next part may be unneded due to mem_root->first_block_usage counter*/
  if ((next->left-= length) < mem_root->min_malloc)
  {           /* Full block */
    *prev= next->next;        /* Remove block from list */
    next->next= mem_root->used;
    mem_root->used= next;   
    mem_root->first_block_usage= 0;
  }  
  
  return (void*)point;
}

```

进行内存分配的时候首先在mem_root的free的链表中进行查找满足*left > length*条件的结点，如果找到则不用进行malloc了，如果找不到则进行malloc；如果当前分配的block的left已经不足够了则放入used链表。

#### free_root

```cpp
/*                         
  Deallocate everything used by alloc_root or just move 
  used blocks to free list if called with MY_USED_TO_FREE
                           
  SYNOPSIS                 
    free_root()            
      root    Memory root  
      MyFlags   Flags for what should be freed: 
                           
        MY_MARK_BLOCKS_FREED  Don't free blocks, just mark them free
        MY_KEEP_PREALLOC  If this is not set, then free also the
                    preallocated block
                           
  NOTES                    
    One can call this function either with root block initialised with
    init_alloc_root() or with a zero()-ed block.            
    It's also safe to call this multiple times with the same mem_root.
*/                         
void free_root(MEM_ROOT *root, myf MyFlags)
{
  ...
}
```

free_root函数的逻辑比较简单就是将内存管理的free以及used链表进行内存释放，当然还有一些内部的细节问题，这里不进行详述。

在内存释放的过程中会使用*TRASH_MEM()*,这是一个宏，这个宏具体会调用valgrind库的一些宏进行内存泄露检查，有兴趣的可以进行研究研究。valgrind工具对于内存的检查还是很给力的。

## 总结

>* MySQL 的底层其实还是使用 malloc 族的API进行内存管理的。
>* MySQL 的内存管理的实现其实通过数据结构USED_MEM以及MEM_ROOT的数据结构就可以窥探大概了，后面的一些具体的API函数实现的解释其实有点画蛇添足。还记得在之前看过一个话：牛X程序员注重结构，平庸的程序员才会注重代码的细节。

