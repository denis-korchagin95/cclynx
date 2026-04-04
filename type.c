#include <assert.h>

#include "type.h"
#include "errors.h"

struct type type_void = {TYPE_KIND_VOID, 0, 0, 0};
struct type type_sint32 = {TYPE_KIND_INTEGER, 4, 4, TYPE_MODIFIER_SIGNED};
struct type type_uint32 = {TYPE_KIND_INTEGER, 4, 4, TYPE_MODIFIER_UNSIGNED};

const char * type_stringify(const struct type * type)
{
    assert(type != NULL);

    switch (type->kind) {
        case TYPE_KIND_VOID:
            return "void";
        case TYPE_KIND_INTEGER:
            if (type->modifiers & TYPE_MODIFIER_UNSIGNED) {
                return "unsigned int";
            }
            return "int";
        default:
            return "<unknown type>";
    }
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
