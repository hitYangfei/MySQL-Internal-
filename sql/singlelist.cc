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
