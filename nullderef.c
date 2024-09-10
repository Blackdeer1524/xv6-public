#include "user.h"

int
main(int argc, char *argv[])
{
	char *test = (char *) 0;
	// either will cause an exception (won't find first page)
	/* *test = 1; */
	/* printf(1, "%d\n", *test); */

	// will cause an exception and kill proc
	/* test = (char *)0x1000; */
	/* mprotect((void *)test, 1); */
	/* *test = 1; */

	// will work fine
	test = (char *)0x1000;
	mprotect((void *)test, 1);
	munprotect((void *)test, 1);
	*test = 1;
	printf(1, "%d\n", *test);

	exit();
}
