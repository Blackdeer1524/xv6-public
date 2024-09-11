#include "user.h"

void 
foo(void *first, void *second) 
{
	int f = *(int *)first;
	int s = *(int *)second;
	printf(1, "first: %d; second: %d\n", f, s);
	exit();
}

int 
main()
{
	int f = 123;
	int s = 256;
	printf(1, "creating a thread...\n");
	int tid = thread_create(foo, &f, &s);
	if (tid < 0) {
		printf(1, "couldn't create a thread\n");
		exit();
	}
	printf(1, "thread: %d. joining...\n", tid);
	if (thread_join() < 0) {
		printf(1, "couldn't join %d\n", tid);
		exit();
	};
	printf(1, "joined\n", tid);
	exit();
}
