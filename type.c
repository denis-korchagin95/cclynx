#include <assert.h>

#include "type.h"
#include "errors.h"

struct type type_void = {TYPE_KIND_VOID, 0, 0, 0};
struct type type_sint32 = {TYPE_KIND_INTEGER, 4, 4, TYPE_MODIFIER_SIGNED};
struct type type_uint32 = {TYPE_KIND_INTEGER, 4, 4, TYPE_MODIFIER_UNSIGNED};
struct type type_float = {TYPE_KIND_FLOAT, 4, 4, 0};

const char * type_stringify(const struct type * type)
{
    assert(type != NULL);

    if (type == &type_void) {
        return "void";
    }

    if (type == &type_float) {
        return "float";
    }

    if (type == &type_sint32) {
        return "int";
    }

    if (type == &type_uint32) {
        return "unsigned int";
    }

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
