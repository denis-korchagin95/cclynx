#include <string.h>

#include "identifier.h"
#include "allocator.h"

#define IDENTIFIER_TABLE_SIZE  (101)

static struct identifier * identifiers[IDENTIFIER_TABLE_SIZE] = {0};

static struct keyword
{
    const char * name;
} keywords[] = {
    {"int"},
    {"return"},
    {"while"},
    {"if"},
    {"else"},
};


unsigned int get_string_hash(const char * name)
{
    unsigned int hash = 0;
    while(*name != '\0') {
        hash = hash * 31 + (unsigned int) *name;
        ++name;
    }
    return hash;
}

struct identifier * identifier_create(const char * name)
{
    const unsigned int hash = get_string_hash(name);
    const unsigned int len = strlen(name);
    return identifier_insert(hash, name, len);
}

struct identifier * identifier_lookup(const char * name)
{
    const unsigned int hash = get_string_hash(name);
    return identifier_find(hash, name);
}

struct identifier * identifier_find(const unsigned int hash, const char * name)
{
    const unsigned int index = hash % IDENTIFIER_TABLE_SIZE;
    for(struct identifier * it = identifiers[index]; it != NULL; it = it->next) {
        if (strcmp(name, it->name) == 0) {
            return it;
        }
    }
    return NULL;
}

struct identifier * identifier_insert(const unsigned int hash, const char * name, unsigned int len)
{
    const unsigned int index = hash % IDENTIFIER_TABLE_SIZE;

    main_pool_alloc(struct identifier, identifier)

    identifier->name = (char *) memory_blob_pool_alloc(&main_pool, len + 1);
    strncpy(identifier->name, name, len);
    identifier->name[len] = '\0';

    identifier->next = identifiers[index];
    identifiers[index] = identifier;
    return identifier;
}

void init_keywords(void)
{
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        struct keyword * keyword = &keywords[i];

        struct identifier * identifier = identifier_create(keyword->name);
        identifier->is_keyword = 1;
    }
}