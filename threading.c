#include "user.h"

static struct lock_t LOCK;

void
bar(void *first, void *second)
{
	lock(&LOCK);
	printf(1, "some long string of text that would be printed on screen from thread\n");
	unlock(&LOCK);
	exit();
}

int 
main()
{
	lock_init(&LOCK);

	printf(1, "created a thread %d\n", thread_create(bar, 0, 0));
	printf(1, "created a thread %d\n", thread_create(bar, 0, 0));

	lock(&LOCK);

	printf(1, "another long string of text that would be printed on screen from thread\n");

	unlock(&LOCK);

	printf(1, "joining...\n");
	printf(1, "joined %d\n", thread_join());
	exit();
}
