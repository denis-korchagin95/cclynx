#ifndef CCLYNX_UTIL_H
#define CCLYNX_UTIL_H 1

#include <stddef.h>

static inline size_t align_up(const size_t size, const size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

#endif /* CCLYNX_UTIL_H */
