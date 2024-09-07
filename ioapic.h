#ifndef _IOAPIC_H_
#define _IOAPIC_H_

#include "types.h"

void            ioapicenable(int irq, int cpu);
extern uchar    ioapicid;
void            ioapicinit(void);

#endif
