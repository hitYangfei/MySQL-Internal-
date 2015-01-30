## 论程序员的工具性

昨天在进行对我们的分布式数据库中间件进行单元测试的时候发现了一个偶现的问题。首先说明一下，我们的分布式数据库中间件产品的单元测试使用MySQL的mtr工具，mtr是用perl写的一套单元测试工具。昨天的测试内容大致是：在一个复杂的主从复制集群中，手动的进行主切换。这个测试功能通过的前提是在手动切换的时候，所有的主从集群的server状态都必须是正常工作状态。

由于在测试中间会进行一些stop server 以及 start server的动作来保证中间件的高可用性，所以在正确进行手动主从切换测试时需要进行sleep xx，睡上几秒以等待集群关系在stop server或者start server后恢复到工作状态，但问题就出现在了这个sleep xx上。那么到底sleep多久会是可靠的呢，根据经验大约6s即可，但是由于机器配置的不同，测试环境的不同，这个数字很难定论，当然了要是不怕浪费时间sleep个一分钟应该肯定没问题。

由于sleep xx穿插在测试的测例中，总是会不经意的出现一些这样那样的测试问题而导致达不到测试目的，那么能不能像mtr 那样写一个wait_xxx.inc的文件进行查询集群状态，正常后再进行测试呢，而不是傻傻的等待。

看了看文档，参考了一下mysql提供的mtr的一些例子，写了下面的一个小工具来等待指定的一个集群的工作状态

```perl
--let $_wait_continue= 1

if ($data_source_name)
{
  --let $wait_sql= `SELECT concat("DBSCALE SHOW DATASOURCE ","$data_source_name")`
  while($_wait_continue)
  {
    --let $_result= query_get_value($wait_sql, $status, 1)
    if ($_result == 'Working')
    {
      --echo [waiting source $data_source_name is working]
      --let $_wait_continue= 0
    }
    sleep 1;
  }
}
```

### 总结

程序员到底是干什么的，其实就是解放双手，让一些自动化，工具话。所以以后问题一定要想出通用解决方案，而不是去不停的增大sleep的时间。
