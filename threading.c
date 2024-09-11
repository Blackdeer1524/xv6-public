#include "user.h"

void 
foo(void *first, void *second) 
{
	printf(1, "first: %p; second: %p\n", first, second);

	int f = *(int *)first;
	int s = *(int *)second;
	printf(1, "[!!!] first: %d; second: %d\n", f, s);
	exit();
}

void
worker(void *first, void *second)
{
	printf(1, "worker: start\n");
	for (int i = 0; i < 1000000000; ++i) {
		asm volatile("" : : : "memory");
	}
	printf(1, "worker: finish\n");
	
	exit();
}

int 
main()
{
	int f = 123;
	int s = 256;
	/* printf(1, "created a thread %d\n", thread_create(foo, &f, &s)); */
	printf(1, "created a thread %d\n", thread_create(worker, &f, &s));
	/* printf(1, "joined %d\n", thread_join()); */
	/* printf(1, "joined %d\n", thread_join()); */
	/* printf(1, "joined %d\n", thread_join()); */
	exit();
}
