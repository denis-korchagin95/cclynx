#include <assert.h>
#include <memory.h>
#include <string.h>

#include "symbol.h"
#include "identifier.h"
#include "allocator.h"
#include "error.h"
#include "type.h"


void init_symbols(struct hashmap * identifier_table, struct memory_blob_pool * pool)
{
    assert(identifier_table != NULL);
    assert(pool != NULL);
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
