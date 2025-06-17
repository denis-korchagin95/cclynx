#ifndef TYPE_H
#define TYPE_H 1

#include <stddef.h>

enum type_kind
{
    TYPE_KIND_INTEGER = 1,
};

struct type
{
    enum type_kind kind;
    size_t size;
    size_t alignment;
};

extern struct type type_integer;

#endif /* TYPE_H */
