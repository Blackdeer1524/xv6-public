#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "file.h"

void cprintf(char *fmt, ...);
void panic(char *) __attribute__((noreturn));
void consoleintr(int (*getc)(void));
int consoleread(struct inode *ip, char *dst, int n);
int consolewrite(struct inode *ip, char *buf, int n);
void consoleinit(void);

#endif
