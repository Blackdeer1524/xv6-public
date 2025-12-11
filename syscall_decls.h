#ifndef _SYSCALL_DECLS_H_
#define _SYSCALL_DECLS_H_

#include "types.h"

int             argint(int, int*);
int             argptr(int, char**, int);
int             argstr(int, char**);
int             fetchint(uint, int*);
int             fetchstr(uint, char**);
void            syscall(void);

#endif
