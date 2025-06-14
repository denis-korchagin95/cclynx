#ifndef IDENTIFIER_H
#define IDENTIFIER_H 1

struct symbol;

#define identifier_attach_symbol(identifier, symbol) \
    (symbol)->next = (identifier)->symbols;          \
    (identifier)->symbols = (symbol);                \

struct identifier
{
    char * name;
    struct symbol * symbols;
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
