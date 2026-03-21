#include <assert.h>

#include "scope.h"
#include "allocator.h"

#include "identifier.h"
#include "symbol.h"

struct scope * scope_push(struct scope * scope, struct memory_blob_pool * pool)
{
    assert(scope != NULL);
    assert(pool != NULL);
    struct scope * new_scope = memory_blob_pool_alloc(pool, sizeof(struct scope));
    memset(new_scope, 0, sizeof(struct scope));
    new_scope->enclosing = scope;
    new_scope->symbols = NULL;
    return new_scope;
}

struct scope * scope_pop(const struct scope * scope)
{
    assert(scope != NULL);
    assert(scope->enclosing != NULL);

    const struct symbol_list * it = scope->symbols;

    while (it != NULL) {
        identifier_detach_symbol(it->symbol->identifier, it->symbol);

        it = it->next;
    }

    return scope->enclosing;
}

void scope_add_symbol(struct scope * scope, struct symbol * symbol, struct memory_blob_pool * pool)
{
    assert(scope != NULL);
    assert(pool != NULL);

    struct symbol_list * new_element = memory_blob_pool_alloc(pool, sizeof(struct symbol_list));
    memset(new_element, 0, sizeof(struct symbol_list));
    new_element->symbol = symbol;
    new_element->next = scope->symbols;
    scope->symbols = new_element;
}

struct symbol * scope_find_symbol(const struct scope * scope, const struct identifier * identifier, enum symbol_kind symbol_kind)
{
    assert(scope != NULL);

    const struct symbol_list * it = scope->symbols;

    while (it != NULL) {
        if (it->symbol->identifier == identifier && it->symbol->kind == symbol_kind)
            return it->symbol;

        it = it->next;
    }

    return NULL;
}