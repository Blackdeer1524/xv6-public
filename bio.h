#ifndef _BIO_H_
#define _BIO_H_

#include "types.h"

void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf *);
void bwrite(struct buf *);

#endif
