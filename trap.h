#ifndef _TRAP_H_
#define _TRAP_H_

#include "types.h"
#include "spinlock.h"

void            idtinit(void);
extern uint     ticks;
void            tvinit(void);
extern struct spinlock tickslock;

#endif
