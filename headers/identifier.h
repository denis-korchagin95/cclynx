#ifndef IDENTIFIER_H
#define IDENTIFIER_H 1

struct identifier
{
    char * name;
    struct identifier * next;
};

struct identifier * identifier_create(const char * name);
struct identifier * identifier_lookup(unsigned int hash, const char * name);
struct identifier * identifier_insert(unsigned int hash, const char * name, unsigned int len);

#endif /* IDENTIFIER_H */
