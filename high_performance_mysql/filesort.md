

## MySQL filesort的理解

在对sql进行调优时候，经常碰到explain输出using filesort,那么这是什么意思的

如果mysql在排序的时候没有使用到索引那么就会输出using filesort。mysql对排序有两种实现

### 两边扫描

第一遍扫描出需要排序的字段，然后进行排序后，根据排序结果，第二遍再扫描一下需要select的列数据。这回引起大量的随即IO，效率不高，但是节约内存。排序使用quick sort。但是如果内存不够则会按照block进行排序，将排序结果写入磁盘文件，然后再将结果合并。

### 一遍扫描

即一遍扫描数据后将select需要的列数据以及排序的列数据都取出来，这样就不需要进行第二遍扫描了，当然内存不足时也会使用磁盘临时文件进行外排。


MySQL根据max_length_for_sort_data来判断排序时使用一遍扫描还是两遍扫描。如果需要的列数据一行可以放入max_length_for_sort_data则使用一遍扫描否则使用两遍扫描

MySQL根据sort_buffer_size来判断是否使用磁盘临时文件，如果需要排序的数据能放入sort_buffer_size则无需使用磁盘临时文件，此时explain只会输出using filesort 否则需要使用磁盘临时文件explain会输出using temporary;using filesort

##总结

当看到MySQL的explain输出using filesort不要太过紧张，这说明排序的时候没有使用索引，如果输出using temporary;using filesort则需要引起注意了，说明使了磁盘临时文件，效率会降低。一句话using filesort需要酌情优化。
