#include <assert.h>
#include <memory.h>

#include "symbol.h"
#include "identifier.h"
#include "allocator.h"
#include "errors.h"
#include "type.h"


static struct builtin_symbol
{
    const char * name;
    enum symbol_kind symbol_kind;
    struct type * type;
} builtin_symbols[] = {
    {"int", SYMBOL_KIND_TYPE_SPECIFIER, &type_integer},
    {"float", SYMBOL_KIND_TYPE_SPECIFIER, &type_float},
};

void init_symbols(struct hashmap * identifier_table, struct memory_blob_pool * pool)
{
    assert(identifier_table != NULL);
    assert(pool != NULL);
    for (size_t i = 0; i < sizeof(builtin_symbols) / sizeof(builtin_symbols[0]); ++i) {
        struct builtin_symbol * builtin_symbol = &builtin_symbols[i];

        struct identifier * identifier = identifier_lookup(identifier_table, builtin_symbol->name);

        if (identifier == NULL) {
            cclynx_fatal_error("ERROR: not found identifier for symbol \"%s\"\n", builtin_symbol->name);
        }

        main_pool_alloc(struct symbol, symbol)
        symbol->kind = builtin_symbol->symbol_kind;
        symbol->type = builtin_symbol->type;
        symbol->identifier = identifier;

        identifier_attach_symbol(pool, identifier, symbol)
    }
}

struct symbol * symbol_lookup(const struct identifier * identifier, enum symbol_kind kind)
{
    assert(identifier != NULL);

    for (const struct symbol_list * it = identifier->symbols; it != NULL; it = it->next) {
        if (it->symbol->kind == kind) {
            return it->symbol;
        }
    }

    return NULL;
}
