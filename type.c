#include <assert.h>

#include "type.h"
#include "errors.h"

struct type type_void = {TYPE_KIND_VOID, 0, 0, 0};
struct type type_integer = {TYPE_KIND_INTEGER, 4, 4, TYPE_MODIFIER_SIGNED};
struct type type_float = {TYPE_KIND_FLOAT, 4, 4, 0};

const char * type_stringify(const struct type * type)
{
    assert(type != NULL);

    if (type->kind == TYPE_KIND_VOID)
        return "void";
    if (type->kind == TYPE_KIND_INTEGER)
        return "int";
    if (type->kind == TYPE_KIND_FLOAT)
        return "float";

    return "<unknown type>";
}

struct type * type_resolve(struct type * lhs, const struct type * rhs)
{
    assert(lhs != NULL);
    assert(rhs != NULL);

    if (lhs->kind != rhs->kind) {
        cclynx_fatal_error("ERROR: type mismatch between '%s' and '%s'\n", type_stringify(lhs), type_stringify(rhs));
    }

    return lhs;
}
