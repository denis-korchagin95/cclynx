#ifndef TYPE_H
#define TYPE_H 1

#include <stddef.h>

enum type_kind
{
    TYPE_KIND_VOID = 0,
    TYPE_KIND_INTEGER,
};

struct type
{
    enum type_kind kind;
    size_t size;
    size_t alignment;
};

extern struct type type_integer;
extern struct type type_void;

#endif /* TYPE_H */
