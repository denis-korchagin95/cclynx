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
};

struct identifier * identifier_create(const char * name);
struct identifier * identifier_lookup(unsigned int hash, const char * name);
struct identifier * identifier_insert(unsigned int hash, const char * name, unsigned int len);

#endif /* IDENTIFIER_H */
