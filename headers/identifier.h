#ifndef CCLYNX_IDENTIFIER_H
#define CCLYNX_IDENTIFIER_H 1

struct hashmap;
struct memory_blob_pool;
struct symbol_list;
struct symbol;

#define identifier_attach_symbol(pool, identifier, symbol_name)             \
    {                                                                       \
        struct symbol_list * element = (struct symbol_list *)                \
            memory_blob_pool_alloc((pool), sizeof(struct symbol_list));      \
        memset(element, 0, sizeof(struct symbol_list));                      \
        element->symbol = (symbol_name);                                    \
        element->next = (identifier)->symbols;                              \
        (identifier)->symbols = element;                                    \
    }

struct identifier
{
    char * name;
    struct symbol_list * symbols;
    unsigned int is_keyword:1;
    unsigned int reserved:31;
};

void init_keywords(struct hashmap * identifier_table, struct memory_blob_pool * pool);

struct identifier * identifier_create(struct hashmap * identifier_table, struct memory_blob_pool * pool, const char * name);
struct identifier * identifier_lookup(struct hashmap * identifier_table, const char * name);
struct identifier * identifier_insert(struct hashmap * identifier_table, struct memory_blob_pool * pool, const char * name, unsigned int len);
void identifier_detach_symbol(struct identifier * identifier, const struct symbol * symbol);

#endif /* CCLYNX_IDENTIFIER_H */
