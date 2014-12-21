# MySQL 源码指针的艺术--sql_list


## 例子引入

```cpp
#include<stdio.h>
   
struct node{
  struct node *next;
  int a;
}; 
int main()
{  
  node node1,node2;
  node1.a=1;
  node2.a=2;
  node2.next=NULL;
  node1.next=&node2;                                                                                                                                  
  node *p = &node1;
  node **q = (node **)p;
  return 1;
} 
```

调试一下在我的机器上运行如下:
<table>
<tr><td>内存地址</td><td>内容</td></tr>
<tr><td>0x7fffffffd9e0</td><td>node1的地址{next = 0x7fffffffd9f0, a = 1}</td></tr>
<tr><td>0x7fffffffd9f0</td><td>node2的地址{next = 0x0, a = 2}</td></tr>
</table>

所以这里p = 0x7fffffffd9e0,十分好理解。我的机器是64位机器所以sizeof(node) = 16,其中指针8,int 4,剩下的4个浪费掉。所以node1 node2之间刚好差两个字节，16位。此时node **q = (node **)p; 会将p原本指向一个16位的结构体struct node强制转化为一个指向struct node * 的8位的二维指针q的值是0x7fffffffd9e0,*q的值为0x7fffffffd9f0，为node2的地址，node.next的值。如果在声明node时候将next与a顺序调换，是得不到这个效果的。下面进入正题

## base_list

MySQL的base_list是一个单向链表的基类，很多的链表都复用这个类。这个类
下面简化一下这个类的首先来分析push_back的实现，代码的深度让人叹为观止

```cpp
#include<stdio.h>
struct list_node 
{
  list_node *next;
  void *info;
  list_node(void *info_par,list_node *next_par)
    :next(next_par),info(info_par)
  {}
  list_node()         /* For end_of_list */
  {
    info= 0;
    next= this;
  }
};
list_node end_of_list;

class base_list
{
public:
  list_node *first,**last;
  unsigned int elements;
  void empty() { elements=0; first= &end_of_list; last=&first;}
  base_list() { empty(); }
  bool push_back(void *info)
  {
    if (((*last)=new list_node(info, &end_of_list)))
    {
      last= &(*last)->next;
      elements++;
      return 0;
    }
    return 1;
  }

};
int main()
{
  int a=1,b=2,c=3,d=4;
  base_list list;
  list.push_back((void*)(&a));
  list.push_back((void*)(&b));
  list.push_back((void*)(&c));
  list.push_back((void*)(&d));
  return 0;
}
```

first指向链表的第一个元素，*last指向最后一个元素，last指向last->next待添加到链表末尾的那个元素,调试一下在我的机器上运行如下:

### 初始化

<table>
<tr><td>内存地址</td><td>内容</td></tr>
<tr><td>0x601050</td><td>end_of_list的地址{next = 0x601050 &lt;end_of_list&gt;, info = 0x0},first、* last的值</td></tr>
<tr><td>0x7fffffffd9f0</td><td>0x601050 ，last的值</td></tr>
</table>

### push_back(a)之后的执行

>(*last)=new list_node(info, &end_of_list) 
>申请一个节点next指向end_of_list, *last的值为0x601050所以first 的值也会更新为这个新申请的节点,
>*last以及first被更新为0x602010,他们的next值为ox601050

>last= &(* last)->next;
>(* last)->next指向end_of_list值为0x601050
>&(* last)->next的值到底是什么呢？即哪一个地址存储的是0x601050这个值

>答案是0x602010
>与上面的解释一样，(list_node * )0x602010是first, * last, (list_node **)0x602010是一个指向list_node * 的指针，刚好这个地址的前八位为first->next
>即( * last)->next 0x601050 所以last=0x602010此时的 * last = 0x601050 即指向了最后一个元素

<table>
<tr><td>内存地址</td><td>内容</td></tr>
<tr><td>0x601050</td><td>end_of_list的地址{next = 0x601050 &ltend_of_list&gt, info = 0x0}</td></tr>
<tr><td>0x602010</td><td>{next = 0x601050 &ltend_of_list&gt, info = &a}, first指向这个</td></tr>
<tr><td>0x602010</td><td>last等于这个地址值，*last = 0x601050 所以要清楚对于 *last的修改就是读first->next的修改</td></tr>
</table>


### push_back(b)之后

<table>
<tr><td>内存地址</td><td>内容</td></tr>
<tr><td>0x601050</td><td>end_of_list的地址{next = 0x601050 &ltend_of_list&gt, info = 0x0}</td></tr>
<tr><td>0x602010</td><td>{next = 0x602030 , info = &a}, first指向这个</td></tr>
<tr><td>0x602030</td><td>{next = 0x601050 &ltend_of_list&gt, info = &b} </td></tr>
<tr><td>0x602030</td><td>last等于这个地址值，* last = 0x601050 所以要清楚对于* last的修改就是读first->next的修改</td></tr>
</table>

接下来的过程重复这个过程完成链表的建立。

