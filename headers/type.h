#ifndef CCLYNX_TYPE_H
#define CCLYNX_TYPE_H 1

#include <stddef.h>

#define TYPE_MODIFIER_SIGNED    (1 << 0)
#define TYPE_MODIFIER_UNSIGNED  (1 << 1)

enum type_kind
{
    TYPE_KIND_UNDEFINED = 0,
    TYPE_KIND_VOID,
    TYPE_KIND_INTEGER,
    TYPE_KIND_FLOAT,
};

struct type
{
    enum type_kind kind;
    size_t size;
    size_t alignment;
    unsigned int modifiers;
};

extern struct type type_sint32;
extern struct type type_uint32;
extern struct type type_void;
extern struct type type_float;

const char * type_stringify(const struct type * type);
struct type * type_resolve(struct type * lhs, const struct type * rhs);

#endif /* CCLYNX_TYPE_H */
