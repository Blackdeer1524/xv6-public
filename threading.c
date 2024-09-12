#include "user.h"

static struct lock_t LOCK;

void
bar(void *first, void *second)
{
	printf(1, "[child] locking...\n");
	lock(&LOCK);
	printf(1, "[child] locking...\n");

	printf(1, "some long string of text that would be printed on screen from thread\n");

	printf(1, "[child] unlocking...\n");
	unlock(&LOCK);
	printf(1, "[child] unlocked\n");
	exit();
}

int 
main()
{
	lock_init(&LOCK);

	printf(1, "created a thread %d\n", thread_create(bar, 0, 0));

	printf(1, "[parent] locking...\n");
	lock(&LOCK);
	printf(1, "[parent] locking...\n");

	printf(1, "another long string of text that would be printed on screen from thread\n");

	printf(1, "[parent] unlocking...\n");
	unlock(&LOCK);
	printf(1, "[parent] unlocked\n");

	printf(1, "joining...\n");
	printf(1, "joined %d\n", thread_join());
	exit();
}
