#ifndef IDENTIFIER_H
#define IDENTIFIER_H 1

struct symbol_list;

#define identifier_attach_symbol(identifier, symbol_name)    \
    {                                                        \
        main_pool_alloc(struct symbol_list, element)         \
        element->symbol = (symbol_name);                     \
        element->next = (identifier)->symbols;               \
        (identifier)->symbols = element;                     \
    }

struct identifier
{
    char * name;
    struct symbol_list * symbols;
    struct identifier * next;
    unsigned int is_keyword:1;
    unsigned int reserved:31;
};

void init_keywords(void);

struct identifier * identifier_create(const char * name);
struct identifier * identifier_lookup(const char * name);
struct identifier * identifier_find(unsigned int hash, const char * name);
struct identifier * identifier_insert(unsigned int hash, const char * name, unsigned int len);

#endif /* IDENTIFIER_H */
