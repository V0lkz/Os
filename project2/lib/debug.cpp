#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>
#include <stdio.h>

#include "uthread.h"

#ifdef DEBUG    // DEBUG ON

#define PRINT(format, ...) fprintf(stderr, format, __VA_ARGS__);

inline void S_PRINT(int iter, const char *format, ...) {
    va_list args;
    va_start(args, format);
    static int i = 0;
    if (i++ % iter == 0) {
        vfprintf(stderr, format, args);
    }
    va_end(args);
}

#else    // DEBUG OFF
#define PRINT(format, ...)
#define S_PRINT(iter, format, ...)
#endif

#endif    // DEBUG_H
