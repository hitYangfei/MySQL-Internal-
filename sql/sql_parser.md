## MySQL Parser
```cpp
start_thread()
  handle_one_connection()
    do_command()
      dispatch_command()
        mysql_parse()
          mysql_execute_command()
            execute_sqlcom_select()
              handle_select()   处理一条select语句
```


### mysql_parse() 

#### 1) 初始化lex_start()  对thd->lex进行初始化
lex是语法解析的重要结构，十分庞大，作用是将一个sql语句解析成一颗语法树进行存储，同时最大化利于下一步进行sql优化的处理.
```cpp
struct LEX: public Query_tables_list {
  SELECT_LEX select_lex;   // select 结构体
}LEX;
typedef class st_select_lex SELECT_LEX; 
class st_select_lex: public st_select_lex_node {
  Item *where;
  Item::cond_result cond_value;

}
```
#### 2) 会进行cache判断，通过函数
```cpp
/*
  Check if the query is in the cache. If it was cached, send it
  to the user.
  
  RESULTS
   0 Query was not cached.
   1 The query was cached and user was sent the result.
  -1 The query was cached but we didn't have rights to use it.
    No error is sent to the client yet.                                                                                                        
  
  NOTE
  This method requires that sql points to allocated memory of size:
  tot_length= query_length + thd->db_length + 1 + QUERY_CACHE_FLAGS_SIZE;
*/
```
Query_cache::send_result_to_client(sql_cache.cc)
来实现。如果cache存在就直接返回结果，不进行语法解析，所以MySQL的cache是在语法解析之前发生的，因此导致了cache的死板，后面会进行介绍。  
#### 3) parse_sql进行语法解析
```cpp
 select sql_no_cache * from t1 where c1 = 1 and c2 = 'yangfei';
 
 
 condition where:
 
  Item_and : List<Item> 
     | Item_func(Item_func_eq) | Item_field  c2的值
                               | Item_String 'yangfei'
 
     | Item_func               | Item_field
                               | Item_int
```
