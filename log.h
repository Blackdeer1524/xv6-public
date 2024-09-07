#ifndef _LOG_H_
#define _LOG_H_

#include "buf.h"

void initlog(int dev);
void log_write(struct buf *);
void begin_op();
void end_op();

#endif
