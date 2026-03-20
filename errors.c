#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"


_Noreturn void cclynx_fatal_error(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
