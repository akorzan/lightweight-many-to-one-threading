#include <stdio.h>
#include <stdlib.h>
#include "lwt.h"

int add5(int x) {
	int five = 5, oh = 1000;
	void *esp, *ebp;
	__asm__ __volatile__ ("movl %%ebp, %%ebx\n\t"
			      "movl %%esp, %%eax"
			      : "=a" (esp), "=b" (ebp)
			      : );
	printf("esp: %x, ebp: %x\n", esp, ebp);
	if (x > oh) {
		lwt_yield(LWT_NULL);
		return x;
	}
	return add5(x + five);
}

int sub5(int x) {
	int five = 5, thousand = -1000;
	if (x < thousand) {
		lwt_yield(LWT_NULL);
		return x;
	}
	return sub5(x - five);
}


int main(void)
{
	lwt_t thread_1 = lwt_create(add5, 10);
	//lwt_t thread_2 = lwt_create(sub5, 10);
	lwt_yield(LWT_NULL);
	printf("%d\n", lwt_join(thread_1));
	lwt_yield(LWT_NULL);
	//printf("%d\n", lwt_join(thread_2));
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	return 0;
}
