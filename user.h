#ifndef _USER_H_
#define _USER_H_

#include "pstat.h"
#include "types.h"
#include "stat.h"

// system calls
int clone(void(*fcn)(void*, void *), void *arg1, void *arg2, void *stack);
int join(void **stack);

int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int getreadcount(void);
int getpinfo(struct pstat *stats);
int setticketscount(int tickets);
// addr has to be page aligned, len > 0
int mprotect(void *addr, int len);
// addr has to be page aligned, len > 0
int munprotect(void *addr, int len);
// threading
int thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2);
int thread_join(void);

struct lock_t {
  int ticket;
  int turn;
};
void lock_init(struct lock_t *lock);
void lock(struct lock_t *lock);
void unlock(struct lock_t *lock);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

#endif
