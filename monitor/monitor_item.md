

说明： 如果没有给出阈值或者阈值建议，则这个监控项目一般是用来进行MySQL或者应用系统进行调优的。


MySQL 监控项

MySQL Threads
 MySQL线程情况
 Threads_connected 当前打开的连接数量
 Threads_running 正在运行的线程
 Threads_cached 缓存的线程数量，如果很大，需要调整参数thread_cache_size
 线程缓存命中率 1-threads_cached/connections   阈值为95%, 如果低于这个阈值则需要调整参数thread_cache_size
 Max_used_connections  最大连接数量

MySQL NetWork
 作为系统优化辅助参考一般不设置阈值
 1.bytes_send   mysql 总发送的字节数量
 2.bytes_received mysql总的接收的字节数量

MySQL select types
 1.select_full_join  ,join操作执行的表扫描次数，如果这个值不是0 则说明应该增加索引加速select,阈值应该为1
 2.select_range_check  对于没有索引的join的索引检查次数 如果这个不是0 则应该考检查表的索引， 阈值为1 
 3.select_scan , 第一个表使用全表扫描的join次数。  阈值为1 即出现新的就告警

MySQL Sorts
 sort_merge_passes 排序过程中的merge数量，如果这个数量过大，应该考虑增加sort_buffer_size。阈值为1 即增加就告警
 sort_range 使用range的排序次数
 sort_rows 排序的总行数
 sort_scan 使用扫描表的排序次数

MySQL Command counts
 MySQL命令次数统计，一般不需要设置阈值
 questions  ，  可以设置阈值为1 即出现新问题就进行告警。
 com_insert
 com_select
 com_update
 com_delete
 com_replace
 com_load
 com_update_multi
 com_replace_select
 com_insert_select
 com_delete_multi

MySQL Temporary Objects
 MySQLl临时表情况监控
 1.created_tmp_tables ， mysql创建的临时表数量，可以与监控项3集合进行参考是否需要调整监控3的参数，一般不需要设置阈值
 2.created_tmp_files ， mysql创建的临时文件数量。一般不需要设置阈值
 3.created_tmp_disk_tables, 阈值设为1 即出现新的磁盘临时表就进行告警， 如果经常遇到遇到这个警告，说明tmp_table_size和max_heap_table_size这个两个参数设置小了应该调大

InnoDB IO
 innodb io性能监控情况，可以根据读写次数准备分析业务的读写比，作为进行系统读写分离负载等辅助参考，一般不需要设置阈值
 1.innodb_data_fsyncs，  Innodb 进行fsync的次数
 2.innodb_data_reads     Innodb 进行read的次数
 3.innodb_data_writes    Innodb 进行write的次数
 4.innodb_log_writes     innodb 日志写的次数

InnoDB buffer pool
 innodb缓存池buffer pool监控情况
 1.命中           ，阈值为95% 低于这个值说明缓存命中低，系统性能不好。
 以下的数据监控项一般不设阈值，收集这些数据的目的是辅助工程师进行MySQL或者系统的优化
 2.innodb_buffer_pool_pages_total， innodb buffer pool总页数量，每一个页为16K。
 3.innodb_buffer_pool_pages_free，                    空闲页
 4.innodb_buffer_pool_pages_dirty，                   脏页
 5.innodb_buffer_pool_pages_data ，                   数据量
 6.innodb_buffer_pool_read_requests，                 读请求
 7.innodb_buffer_pool_write_requests                  写请求

InnoDB row operations
 innodb的性能监控，行操作数量，下面四个监控项很直观，分别为读的行数，插入的行数，更新的行数，删除的行数。目的是为了了解系统的性能情况。一般不需要设置具体的告警阈值。
 1.innodb_rows_read
 2.innodb_rows_inserted
 3.innodb_rows_updated
 4.innodb_rows_deleted


SQL状态
 SQL状态的输出为SQL语句或者一些其他的字符串。
 1.执行次数最多的SQL ， 对于执行多的SQL应该考虑是否真的有必要去执行，是否会带来性能问题
 2.执行时间最长的SQL ， 对于执行慢的SQL应该考虑进行SQL调优
 3.从没有被用过的索引， 对于没有使用过过的索引应该考虑删除掉这些索引，索引会代理插入的性能下降，索引少可以提高插入性能
 4.磁盘I/O消耗最多的文件 ，可以考虑将I/O消耗多的文件存储在单独的性能好的磁盘上。


内存
 1.系统内存使用率 , 阈值为70%--80% 
 2.MySQL内存使用率, 根据MySQL在机器上是否独占而定，如果机器只是为了运行MySQL则阈值设置为70%--80% 否则根据实际情况，MySQL的比重来设置。
 3.swap使用情况， 阈值为0,swap在MySQL服务器上不应该被使用。一旦使用就需要引起注意可能会导致性能下降

操作系统CPU
 1.进程总数量   ，  阈值根据系统的负载情况而定，建议值500。
 2.处于运行状态的进程数量， 阈值根据系统的CPU核数而定，一般为核数的70%--80%
 3.系统CPU使用率，   阈值为70%--80% 
 4.MySQL CPU使用率， 根据MySQL在机器上是否独占而定，如果机器只是为了运行MySQL则阈值设置为70%--80% 否则根据实际情况，MySQL的比重来设置。

磁盘I/O :
 监控磁盘I/O的负载情况，提供以下监控项
 1.每秒写盘字节数，  阈值设置为磁盘写性能的70%--80%
 2.每秒读盘字节数，  阈值设置为磁盘读性能的70%--80%
 3.I/O CPU使用率 ，  阈值设置为80%， 这个值偏大说明I/O设备的压力很大，应该考虑更换I/O设备或者对系统进行I/O调优
 4.磁盘剩余空间  ，  一般为数据库数据文件所在磁盘的磁盘剩余空间，阈值为磁盘容量的10%-20%，如果低于这个值说明应该增加磁盘了
 5.磁盘Inode剩余空间， 阈值为磁盘Inode容量的70%--80% ,这个值超过阈值说明小文件数量过多。



网络流量 :
 网络流量主要监控线上系统的网络负载，提供两个监控项
 1.每秒接收数据量
 2.每秒发送数据量
 告警阈值可以设置为流量峰值的70%--80%之间。或者设置为网卡接收包性能的70%--80%。




