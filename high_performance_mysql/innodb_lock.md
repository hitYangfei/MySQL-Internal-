
## InnoDB 锁模式

本文主要涉及innodb为了实现事物的隔离性采用的record lock,gap lock 以及next-key lock

### 一致性读和幻读问题

```SQL
create table t1 (c1 int, c2 int, primary key(c1), key(c2));
insert into t1 values(1,1),(3,3),(5,5),(7,7);
```

```SQL
mysql> begin;
mysql> select * from t1 where c1 > 3 for update;
+----+------+
| c1 | c2   |
+----+------+
|  5 |    5 |
|  7 |    7 |
+----+------+
2 rows in set (0.00 sec)

                                                                         mysql> begin; update t1 set c2=6 where c1=5;commit;

mysql> select * from t1 where c1 > 3 for update;
+----+------+                                      
| c1 | c2   |                                      
+----+------+                                      
|  5 |    6 |                                      
|  7 |    7 |                                      
+----+------+                                      
2 rows in set (0.00 sec)
```
上面这个问题叫做一致性读问题，即在一个事物中两次相同的select结果不相同。这里的不相同是指之前select的数据被修改了。如果多了则叫做幻读问题。看下面的片段,数据回到一开始

```SQL
mysql> begin;
mysql> select * from t1 where c1 > 3 for update;
+----+------+
| c1 | c2   |
+----+------+
|  5 |    5 |
|  7 |    7 |
+----+------+
2 rows in set (0.00 sec)

                                                                         mysql> begin; insert into t1 values(6,6);commit;

mysql> select * from t1 where c1 > 3 for update;
+----+------+                                      
| c1 | c2   |                                      
+----+------+                                      
|  5 |    5 |
|  6 |    6 |
|  7 |    7 |                                      
+----+------+                                      
2 rows in set (0.00 sec)
```
上面这个问题叫做幻读问题。即读到了事物开始的时候不存在的数据。

### inndob的gap lock

mysql的默认隔离基本为repeatable read。但是Innodb不存在幻读问题，这是因为Innodb采用gap-lock锁机制。在上面的操作过程中，当执行
```SQL
select * from t1 where c1>3 for update；
```
innodb 会对(3,5],(5,7],(7,+∞）进行上锁，其中(5,7]这种类型的锁叫做next-key lock

可以分解为(5,6),7这样的两个锁，(5,6)叫做gap-lock ,7 叫做record lock。

所以上面insert会阻塞，从而解决幻读问题。

可以通过将隔离级别降低为read commited 或者开启innodb_locks_unsafe_for_binlog参数，将关闭gap-lock
