#ifndef _IPE_H_
#define _IPE_H_

#include "file.h"
#include "spinlock.h"
#include "types.h"

#define PIPESIZE 512

struct pipe {
	struct spinlock lock;
	char data[PIPESIZE];
	uint nread;    // number of bytes read
	uint nwrite;   // number of bytes written
	int readopen;  // read fd is still open
	int writeopen; // write fd is still open
};

int pipealloc(struct file **, struct file **);
void pipeclose(struct pipe *, int);
int piperead(struct pipe *, char *, int);
int pipewrite(struct pipe *, char *, int);

#endif
