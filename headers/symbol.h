#ifndef CCLYNX_SYMBOL_H
#define CCLYNX_SYMBOL_H 1

#define MAX_SYMBOL_FUNCTION_PARAMETER_COUNT (3)

struct hashmap;
struct memory_blob_pool;
struct type;

enum symbol_kind
{
    SYMBOL_KIND_TYPE_SPECIFIER = 1,
    SYMBOL_KIND_VARIABLE,
    SYMBOL_KIND_FUNCTION,
    SYMBOL_KIND_FUNCTION_PARAMETER,
};

struct symbol
{
    struct identifier * identifier;
    struct type * type;
    enum symbol_kind kind;
    unsigned int parameter_count; /* SYMBOL_KIND_FUNCTION only */
    unsigned int parameter_index; /* SYMBOL_KIND_FUNCTION_PARAMETER only */
    struct symbol * parameters[MAX_SYMBOL_FUNCTION_PARAMETER_COUNT]; /* SYMBOL_KIND_FUNCTION_PARAMETER only */
};

struct symbol_list
{
    struct symbol * symbol;
    struct symbol_list * next;
};

void init_symbols(struct hashmap * identifier_table, struct memory_blob_pool * pool);
struct symbol * symbol_lookup(const struct identifier * identifier, enum symbol_kind kind);

#endif /* CCLYNX_SYMBOL_H */
