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

enum keyword_code
{
    KEYWORD_NONE = 0,
    KEYWORD_VOID,
    KEYWORD_INT,
    KEYWORD_RETURN,
    KEYWORD_WHILE,
    KEYWORD_UNSIGNED,
    KEYWORD_IF,
    KEYWORD_ELSE,
};

struct identifier
{
    char * name;
    struct symbol_list * symbols;
    enum keyword_code keyword_code;
};

void init_keywords(struct hashmap * identifier_table, struct memory_blob_pool * pool);

struct identifier * identifier_create(struct hashmap * identifier_table, struct memory_blob_pool * pool, const char * name);
struct identifier * identifier_lookup(struct hashmap * identifier_table, const char * name, unsigned int len);
struct identifier * identifier_insert(struct hashmap * identifier_table, struct memory_blob_pool * pool, const char * name, unsigned int len);
void identifier_detach_symbol(struct identifier * identifier, const struct symbol * symbol);

#endif /* CCLYNX_IDENTIFIER_H */
