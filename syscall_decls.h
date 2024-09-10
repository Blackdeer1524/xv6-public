#ifndef _SYSCALL_DECLS_H_
#define _SYSCALL_DECLS_H_

#include "types.h"

int  argint(int n, int *ip);
int  argptr(int n, char **pp, int size);
int  argstr(int n, char **pp);
int  fetchint(uint addr, int *ip);
int  fetchstr(uint addr, char **pp);
void syscall(void);

#endif
