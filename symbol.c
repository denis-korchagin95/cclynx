#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "symbol.h"
#include "identifier.h"
#include "allocator.h"


static struct builtin_symbol
{
    const char * name;
    enum symbol_kind kind;
} builtin_symbols[] = {
    {"int", SYMBOL_KIND_TYPE_SPECIFIER},
};

void init_symbols(void)
{
    for (int i = 0; i < sizeof(builtin_symbols) / sizeof(builtin_symbols[0]); ++i) {
        struct builtin_symbol * builtin_symbol = &builtin_symbols[i];

        struct identifier * identifier = identifier_lookup(builtin_symbol->name);

        if (identifier == NULL) {
            fprintf(stderr, "ERROR: not found identifier for symbol \"%s\"\n", builtin_symbol->name);
            exit(1);
        }

        struct symbol * symbol = (struct symbol *) memory_blob_pool_alloc(&main_pool, sizeof(struct symbol));
        memset(symbol, 0, sizeof(struct symbol));
        symbol->kind = builtin_symbol->kind;
        symbol->identifier = identifier;

        identifier_attach_symbol(symbol->identifier, symbol);
    }
}

struct symbol * symbol_lookup(const struct identifier * identifier, enum symbol_kind kind)
{
    assert(identifier != NULL);

    for (struct symbol * it = identifier->symbols; it != NULL; it = it->next) {
        if (it->kind == kind)
            return it;
    }

    return NULL;
}
