#include "user.h"

int 
main(int argc, char *argv[])
{
	int test = *(char *)0x1000;
	printf(1, "%d\n", test);
	exit();
}
