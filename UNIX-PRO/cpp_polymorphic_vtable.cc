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
