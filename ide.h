#ifndef _IDE_H_
#define _IDE_H_

#include "buf.h"

void ideinit(void);
void ideintr(void);
void iderw(struct buf *);

#endif
