#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <unistd.h>

static jmp_buf producer_jmp_buf;
static jmp_buf consumer_jmp_buf;

int data;

int i;
int j;

void producer_coroutine() {

  for (;;) {

    if (! setjmp(producer_jmp_buf)) {
      return;  // first run
    }

    data = i;
    i++;
    longjmp(consumer_jmp_buf, 1);
  }
}

void recur_coroutine(int x) {
 
  if (x <= 0) {
    return;
  }

  recur_coroutine(x - 1);
  printf("%d", );
}

int main(int argc, char **argv) {
  recur_coroutine();
  
  //longjmp(producer_jmp_buf, 1);
}
