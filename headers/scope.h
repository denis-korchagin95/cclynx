#ifndef CCLYNX_SCOPE_H
#define CCLYNX_SCOPE_H 1

#include "symbol.h"

struct memory_blob_pool;

struct scope
{
    struct scope * enclosing;
    struct symbol_list * symbols;
};

struct scope * scope_push(struct scope * scope, struct memory_blob_pool * pool);
struct scope * scope_pop(const struct scope * scope);
void scope_add_symbol(struct scope * scope, struct symbol * symbol, struct memory_blob_pool * pool);
struct symbol * scope_find_symbol(const struct scope * scope, const struct identifier * identifier, enum symbol_kind symbol_kind);

#endif /* CCLYNX_SCOPE_H */
