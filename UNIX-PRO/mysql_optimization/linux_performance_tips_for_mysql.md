## linux上 MySQL 调优的一些建议

###文件系统

1. 采用ext3或者xfs，使用notime方式进行挂载

 当文件被创建，修改和访问时，Linux系统会记录这些时间信息。当系统的读文件操作频繁时，记录文件最近一次被读取的时间信息，将是一笔不少的开销。所以，为了提高系统的性能，我们可以在读取文件时不修改文件的atime属性。可以通过在加载文件系统时使用notime选项来做到这一点。当以noatime选项加载（mount）文件系统时，对文件的读取不会更新文件属性中的atime信息。设置noatime的重要性是消除了文件系统对文件的写操作，文件只是简单地被系统读取。由于写操作相对读来说要更消耗系统资源，所以这样设置可以明显提高服务器的性能。注意wtime信息仍然有效，任何时候文件被写，该信息仍被更新。

比如我要在根文件系统使用noatime，可以编辑/etc/fstab文件，如下：

>       proc /proc proc defaults 0 0
>       none /dev/pts devpts gid=5,mode=620 0 0
>       /dev/md0 /boot ext3 defaults 0 0
>       /dev/md1 none swap sw 0 0
>       /dev/md2 / ext3 defaults,noatime 0 0
>           要使设置立即生效，可运行：
>
>           mount -o remount /
>               这样以后系统读取 / 下的文件时将不会再修改atime属性。

2. IO调用采用noop 或者 deadline方式

参考2是percona给出的一份关于IO调用的不同策略的TPCC测试结果。

###内存

1. Swappiness

>echo 0&gt;/proc/sys/vm/swappiness
>add "vm.swppiness=0" to /etc/sysctl.conf

vm.swappiness 的值表示系统对于 Swap 分区的使用策略。等于0表示最大限度的使用物理内存，然后才是 Swap 空间；等于100表示表示积极的使用 Swap 空间， 并且把内存上的数据及时的搬运到 Swap 空间上。MySQL 优化建议指定为0，最大使用内存。

2. NUMA

使用numa的interleave all 模式

>设置numactl --interleave=all后启动mysqld

NUMA(None Uniform Memory Architecture)是对UMA而言的，UMA就是传统SMP架构的多核处理其的内存访问模式，即所有内存作为一个整体供所有处理器进行使用，而NUMA则将内存进行分块，分配给具体的处理器进行使用，这样可以提高内存访问速度，但是不足就是应用需要大量内存时，内存不足的情况。通过上面的设置可以将内存模式变为round robin方式，即当内存不足时将会使用其他处理器的内存。


更多关于NUMA以及SWAP的讨论和应用可以参考7，这是一个twitter的MySQL分支版本关于NUMA以及SWAP方面的修改以及改进。分支版本托管在github上 https://github.com/twitter/mysql。

###CPU

不要开启省电(powersave)模式。

通过`/proc/cpuinfo`查看cpu信息，通过`/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`查看cpu的模式。

###参考

1. http://www.percona.com/blog/2013/12/07/linux-performance-tuning-tips-mysql/

2. http://www.percona.com/blog/2009/01/30/linux-schedulers-in-tpcc-like-benchmark/

3. http://blog.csdn.net/21aspnet/article/details/6584792

4. http://blog.csdn.net/tianlangxiaoyue/article/details/7249484

5. http://www.qingran.net/2011/09/numa%E5%BE%AE%E6%9E%B6%E6%9E%84/

6. http://blog.csdn.net/zhenwenxian/article/details/6196943

7. http://blog.jcole.us/2012/04/16/a-brief-update-on-numa-and-mysql/
