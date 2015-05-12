
#include <string.h>
#include <errno.h>

#include <zookeeper/zookeeper.h>

static zhandle_t *zh;
static zhandle_t *zh2;
 
/**
 *  * In this example this method gets the cert for your
 *   *   environment -- you must provide
 *    */
char *foo_get_cert_once(char* id) { return 0; }
 
/** Watcher function -- empty for this example, not something you should
 *  * do in real code */
void watcher(zhandle_t *zzh, int type, int state, const char *path,
                 void *watcherCtx) 
{
    if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      printf("DBScale connected to zookeeper service successfully!\n");
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
      printf("DBScale zookeeper session expired!\n");
    } else if (state == ZOO_AUTH_FAILED_STATE) {
      printf("DBScale zookeeper session auth fail!\n");
    }
  }
}
 


int main()
{
  char buffer[512];
  char p[2048];
  char *cert=0;
  char appId[64];
 
  strcpy(appId, "example.foo_test");
  cert = foo_get_cert_once(appId);
  if(cert!=0) {
    fprintf(stderr,
            "Certificate for appid [%s] is [%s]\n",appId,cert);
    strncpy(p,cert, sizeof(p)-1);
    free(cert);
  } else {
    fprintf(stderr, "Certificate for appid [%s] not found\n",appId);
    strcpy(p, "dummy");
  }
 
  zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
 
  zh = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);
  zh2 = zookeeper_init("127.0.0.1:2181", watcher, 10000, 0, 0, 0);
  // zookeeper_init是一个异步函数需要等待zh->state非0
  while(zoo_state(zh) == 0) {}
  while(zoo_state(zh2) == 0) {}

// struct Id id_tmp = {(char*)"digest", (char*)"user:tpUq/4Pn5A64fVZyQ0gOJ8ZWqkY="};

  int rc = zoo_add_auth(zh,"digest", "user:password",strlen("user:password"),0,0);
  if (rc == ZOK){
    printf("zoo add auth sucess\n");
  } else {
    printf("zoo add auth failed\n");
  }
 // struct ACL CREATE_ONLY_ACL[] = {{ZOO_PERM_ALL, id_tmp}};//, {ZOO_PERM_READ, ZOO_AUTH_IDS}, {ZOO_PERM_WRITE, ZOO_AUTH_IDS}};
//  struct ACL_vector CREATE_ONLY = {1, CREATE_ONLY_ACL};
  rc = zoo_create(zh,"/xyz","value", 5, &ZOO_CREATOR_ALL_ACL/*&CREATE_ONLY*/, ZOO_EPHEMERAL,
                      buffer, sizeof(buffer)-1);
 
  if (rc == ZOK) {
    printf("create ok \n");
  } else {
    printf("create faild%d\n" ,rc);
  }

  struct ACL_vector ret;
  printf("zoo set acl %d\n", zoo_get_acl(zh, "/xyz", &ret, 0));
  rc = zoo_exists(zh, "/xyz", 0, 0);
  if (rc != ZOK) {
    printf("%d exist fail\n", rc);
  } else {
    printf("/xyz is exist\n");
  }
  /** this operation will fail with a ZNOAUTH error */
  int buflen= sizeof(buffer);
  struct Stat stat;
  rc = zoo_exists(zh2, "/xyz", 0, 0);
if (rc == ZNOAUTH) {
    fprintf(stderr, "Error %d for %d\n", rc, __LINE__);
  }
  else {
    printf("zh2 exist %s\n", buffer);
  }

  rc = zoo_get(zh2, "/xyz", 0, buffer, &buflen, &stat);
  if (rc == ZNOAUTH) {
    fprintf(stderr, "Error %d for %d\n", rc, __LINE__);
  }
  else {
    printf("get data %s\n", buffer);
  }
  rc = zoo_add_auth(zh2,"digest", "user:password",strlen("user:password"),0,0);
  if (rc == ZOK){
    printf("zoo add auth sucess\n");
  } else {
    printf("zoo add auth failed\n");
  }
  rc = zoo_get(zh2, "/xyz", 0, buffer, &buflen, &stat);
  if (rc == ZNOAUTH) {
    fprintf(stderr, "Error %d for %d\n", rc, __LINE__);
  }
  else {
    printf("get data %s\n", buffer);
  }

  rc = zoo_set(zh, "/xyz", "yangfei", strlen("yangfei"), -1);
  if (rc == ZNOAUTH) {
    fprintf(stderr, "Error %d for %d\n", rc, __LINE__);
  }
  else {
    printf("set data %s\n", buffer);
  }

  rc = zoo_get(zh, "/xyz", 0, buffer, &buflen, &stat);
  if (rc == ZNOAUTH) {
    fprintf(stderr, "Error %d for %d\n", rc, __LINE__);
  }
  else {
    printf("get data %s\n", buffer);
  }
 
   
  zookeeper_close(zh);
  return 0;
}
