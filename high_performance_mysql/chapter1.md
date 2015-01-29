## MySQL结构和历史

### 事务

* A  原子性
* C  一致性
* I  隔离性
* D  持久性

### mysql isolation

mysql支持4个SQL标准的隔离级别

* READ UNCOMMITED  :一个事务会读到另一个事务未提交的数据，称为脏读问题
* READ COMMITED    :一个事务连续执行select会返回不同结果，在这之间另一个事务进行了一些操作并成功提交了，称为不可重复读问题
* REPEATABLE READ  :解决了可重复读问题，但是在第一次读之后第二次读之间，另一个事务做了一些insert操作，这时读不到新insert的数据，称为幻行问题。InnoDB以及XtraDB使用MVCC加上行级别的锁来解决这一问题
* SERIALIZABLE     :完全的串行化，最安全

常用的隔离级别为1,2第一个和最后一个都不常用，mysql默认为REPEATABLE READ,通过设置tx_isolation来进行修改。

>set global tx_isolation=1; //READ COMMITED

### mysql 存储引擎

#### mysiam

mysiam存储引擎将数据文件以及索引文件分开存储，表锁级别，可以进行检查以及修复，支持索引以及全文检索，延迟更新索引键

如果指定了DELAY_KEY_WRITE选项则会开启延迟更新索引键特性，此时在有限将数据索引内容插入到内存中，周期性的进行刷盘，所以存在索引数据丢失的情况，这时可以使用修复特性对索引文件进行修复。

如果表在创建后不再进行修改操作，则这样的表或者适合mysiam压缩表可以使用mysiampack工具进行打包，可以减少I/O提高性能

### mysql 转换表的引擎

* alter table

  >ALTER TABLE mytable ENGINE = InnoDB;

  alter语句可以适用各种引擎之间的转化，但是需要执行很长时间，而且需要用到临时表，同时会对原表进行上锁。需要消耗大量的I/O

* 导入已经导出

  使用mysqldump工具将源表导出为a.sql，然后修改a.sql的建表语句。

* insert select
  
  建立一张新表，然后使用insert select进行数据转换，如果数据量大可以一点一点的进行。

  >CREATE TABLE innodb_table LIKE myisam_table;
  >ALTER TABLE innodb_table ENGINE=InnoDB;
  >INSERT INTO innodb_table SELECT * FROM myisam_table ;

Percona Toolkit 提供了 pt-online-schema-change\(给予Facebook的在线变更shcema技术\)的工具,

### 选择合适的存储引擎

需要考虑到事物、热备份、崩溃恢复以及一些存储引擎的特性

#### 日志性应用

日志数据可以使用mysiam或者archive存储引擎，因为他们开销低插入速度快。

#### 订单处理

肯定需要事务，InnoDB。

>select count(*)在innodb上很慢。而mysiam上是很快的，因为mysiam表的元数据存储着表的行数。

#### 大数据量

单机在10T以下使用InnoDB，否则需要建立数据仓库。Infobright或者TokuDB都是不错的选择。

不清楚为什么高性能会得出这样的结论，可能是InnoDB的共享表空间有关系，有待后续补充。
