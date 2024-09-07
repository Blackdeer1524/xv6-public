#ifndef _LAPIC_H_
#define _LAPIC_H_

#include "types.h"
#include "date.h"


void            cmostime(struct rtcdate *r);
int             lapicid(void);
extern volatile uint*    lapic;
void            lapiceoi(void);
void            lapicinit(void);
void            lapicstartap(uchar, uint);
void            microdelay(int);

#endif
