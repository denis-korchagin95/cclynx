#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "symbol.h"
#include "identifier.h"
#include "allocator.h"
#include "type.h"


static struct builtin_symbol
{
    const char * name;
    enum symbol_kind symbol_kind;
    struct type * type;
} builtin_symbols[] = {
    {"int", SYMBOL_KIND_TYPE_SPECIFIER, &type_integer},
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

        main_pool_alloc(struct symbol, symbol)
        symbol->kind = builtin_symbol->symbol_kind;
        symbol->type = builtin_symbol->type;
        symbol->identifier = identifier;

        identifier_attach_symbol(identifier, symbol)
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
