#ifndef _ASSERT_H_
#define _ASSERT_H_

#include "console.h"
#include "proc.h"

#define ASSERT(cond, mes, ...)                                                 \
    {                                                                          \
        if (!(cond)) {                                                         \
            cprintf(mes __VA_OPT__(, ) __VA_ARGS__);                           \
            procdump();                                                        \
            panic("assertion error");                                          \
        }                                                                      \
    }

#endif
