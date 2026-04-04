#ifndef CCLYNX_TYPE_H
#define CCLYNX_TYPE_H 1

#include <assert.h>
#include <stddef.h>

#define TYPE_MODIFIER_SIGNED    (1 << 0)
#define TYPE_MODIFIER_UNSIGNED  (1 << 1)
#define TYPE_MODIFIER_SIGN_MASK (TYPE_MODIFIER_SIGNED | TYPE_MODIFIER_UNSIGNED)

enum type_kind
{
    TYPE_KIND_UNDEFINED = 0,
    TYPE_KIND_VOID,
    TYPE_KIND_INTEGER,
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

static inline int type_signedness_differs(const struct type * a, const struct type * b)
{
    assert(a != NULL);
    assert(b != NULL);

    return (a->modifiers & TYPE_MODIFIER_SIGN_MASK) != (b->modifiers & TYPE_MODIFIER_SIGN_MASK);
}

const char * type_stringify(const struct type * type);
struct type * type_resolve(struct type * lhs, const struct type * rhs);

#endif /* CCLYNX_TYPE_H */
