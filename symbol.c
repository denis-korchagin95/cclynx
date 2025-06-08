#include <assert.h>
#include <memory.h>

#include "symbol.h"
#include "identifier.h"
#include "allocator.h"


struct builtin_symbol
{
    const char * name;
    enum symbol_kind kind;
} builtin_symbols[] = {
    {"int", SYMBOL_KIND_KEYWORD},
    {"return", SYMBOL_KIND_KEYWORD},
    {"while", SYMBOL_KIND_KEYWORD},
    {"if", SYMBOL_KIND_KEYWORD},
    {"else", SYMBOL_KIND_KEYWORD},
};

void init_builtin_symbols(void)
{
    for (int i = 0; i < sizeof(builtin_symbols) / sizeof(builtin_symbols[0]); ++i) {
        struct builtin_symbol * builtin_symbol = &builtin_symbols[i];

        struct symbol * symbol = (struct symbol *) memory_blob_pool_alloc(&main_pool, sizeof(struct symbol));
        memset(symbol, 0, sizeof(struct symbol));
        symbol->kind = builtin_symbol->kind;
        symbol->identifier = identifier_create(builtin_symbol->name);

        identifier_attach_symbol(symbol->identifier, symbol);
    }
}

struct symbol * symbol_lookup(struct identifier * identifier, enum symbol_kind kind)
{
    assert(identifier != NULL);

    for (struct symbol * it = identifier->symbols; it != NULL; it = it->next) {
        if (it->kind == kind)
            return it;
    }

    return NULL;
}
