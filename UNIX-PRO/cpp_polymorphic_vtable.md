

## c++ 多态实浅析
今天大企鹅问得一道面试题没答上来，总结一下，看下面的多态例子。
```cpp
#include <stdio.h>                                                                                                                                    
 
class parent {
public:
  int i;
  parent() {
    i = 1;
    printf("parent init\n");
  }
  virtual void vt() {
    printf("this is parent\n");
  }
  virtual void vt1() {
    printf(" parent vt1\n");
  }
};
class child : public parent {
public:
  int j;
  child() {
    j = 2;
    printf("child init\n");
  }
  void vt() {
    printf("this is child\n");
  }
  void vt1() {
    printf(" child vt1\n");
  }
};
 
int main() {
  child a;
  parent b;
  parent *A = &a;
  A->vt();
  return 1;
} 
```

结果很显然，会调用子类的实现。输出"this is child\n"。那么C++是如何实现动态的呢。

对于每一个C++的类，如果有virtual函数则会建议一个vtable，这里存放着所有的虚函数指针，vtable是以类为单位的，不是对象，在类的构造函数中进行初始化（精确时刻不太了解，大概是在构造函数调用的这个期间）。下面看一下上面代码的调试信息。

```cpp
a的gdb print信息
{<parent> = {_vptr.parent = 0x4008d0 <vtable for child+16>, i = 1}, j = 2}
b的gdb print信息
{_vptr.parent = 0x4008f0 <vtable for parent+16>, i = 1}
*A的gdb print信息
{_vptr.parent = 0x4008d0 <vtable for child+16>, i = 1}
```

可以看到A的编译类型为parent，所以输出A的信息中含有i没有j，但是vtable信息指向child的，因为在运行时A的真实类型为child。从而实现多态。
