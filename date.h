#ifndef _DATE_H_
#define _DATE_H_ 

#include "types.h"

struct rtcdate {
  uint second;
  uint minute;
  uint hour;
  uint day;
  uint month;
  uint year;
};

#endif
