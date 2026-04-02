#ifndef CCLYNX_SYMBOL_H
#define CCLYNX_SYMBOL_H 1

#include <stdint.h>
#include "source.h"

#define MAX_SYMBOL_FUNCTION_PARAMETER_COUNT (3)

struct hashmap;
struct memory_blob_pool;
struct type;

enum symbol_kind
{
    SYMBOL_KIND_TYPE_SPECIFIER = 1,
    SYMBOL_KIND_VARIABLE,
    SYMBOL_KIND_FUNCTION,
};

#define SYMBOL_FLAG_FUNCTION_PARAMETER (1 << 0)
#define SYMBOL_FLAG_USED              (1 << 1)

struct symbol
{
    struct identifier * identifier;
    struct type * type;
    enum symbol_kind kind;
    unsigned int flags;
    struct source_position declaration_position;
    unsigned int parameter_count; /* SYMBOL_KIND_FUNCTION only */
    unsigned int parameter_index; /* SYMBOL_FLAG_FUNCTION_PARAMETER only */
    int parameter_presence; /* SYMBOL_KIND_FUNCTION only */
    struct symbol * parameters[MAX_SYMBOL_FUNCTION_PARAMETER_COUNT]; /* SYMBOL_KIND_FUNCTION only */
};

struct symbol_list
{
    struct symbol * symbol;
    struct symbol_list * next;
};

void init_symbols(struct hashmap * identifier_table, struct memory_blob_pool * pool);
struct symbol * symbol_lookup(const struct identifier * identifier, enum symbol_kind kind);

#endif /* CCLYNX_SYMBOL_H */
