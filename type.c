#include <assert.h>

#include "type.h"

struct type type_void = {TYPE_KIND_VOID, 0, 0};
struct type type_integer = {TYPE_KIND_INTEGER, 4, 4};
struct type type_float = {TYPE_KIND_FLOAT, 4, 4};

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
