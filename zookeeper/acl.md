


## zookeeper权限认证

zookeeper通过自己的ACL模块来实现权限认证。
这里只讨论digest的情况。

通过zoo_add_auth()明文传输帐号密码。
 zoo_add_auth(zh,"digest", "user:password",strlen("user:password"),0,0);
客户端zoo_create需要一个acl认证列表，如果采用digest的方式，格式为user:base64(sha1(password))的方式进行传输
```c
struct Id id_tmp = {(char*)"digest", (char*)"user:tpUq/4Pn5A64fVZyQ0gOJ8ZWqkY="};
struct ACL CREATE_ONLY_ACL[] = {{ZOO_PERM_ALL, id_tmp}}
zoo_create(zh,"/xyz","value", 5, &CREATE_ONLY_ACL, ZOO_EPHEMERAL,
              buffer, sizeof(buffer)-1);
```              
编译命令
gcc zookeeper_acl.c  -lzookeeper_mt
