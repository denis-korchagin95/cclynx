#include <string.h>

#include "identifier.h"
#include "hashmap.h"
#include "allocator.h"

#define IDENTIFIER_TABLE_SIZE  (101)

static struct hashmap identifier_table;

static struct keyword
{
    const char * name;
} keywords[] = {
    {"int"},
    {"float"},
    {"return"},
    {"while"},
    {"if"},
    {"else"},
};


struct identifier * identifier_create(const char * name)
{
    struct identifier * existing = hashmap_find(&identifier_table, name);

    if (existing != NULL) {
        return existing;
    }

    return identifier_insert(name, strlen(name));
}

struct identifier * identifier_lookup(const char * name)
{
    return hashmap_find(&identifier_table, name);
}

struct identifier * identifier_insert(const char * name, unsigned int len)
{
    main_pool_alloc(struct identifier, identifier)

    identifier->name = (char *) memory_blob_pool_alloc(main_pool, len + 1);
    strncpy(identifier->name, name, len);
    identifier->name[len] = '\0';

    hashmap_insert(&identifier_table, identifier->name, identifier);
    return identifier;
}

void init_keywords(void)
{
    hashmap_init(&identifier_table, IDENTIFIER_TABLE_SIZE);

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        const struct keyword * keyword = &keywords[i];

        struct identifier * identifier = identifier_create(keyword->name);
        identifier->is_keyword = 1;
    }
}
