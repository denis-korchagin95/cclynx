#include <assert.h>
#include <memory.h>
#include <string.h>

#include "identifier.h"
#include "hashmap.h"
#include "allocator.h"

#define IDENTIFIER_TABLE_SIZE  (101)

static struct keyword
{
    const char * name;
} keywords[] = {
    {"int"},
    {"float"},
    {"void"},
    {"return"},
    {"while"},
    {"if"},
    {"else"},
};


struct identifier * identifier_create(struct hashmap * identifier_table, struct memory_blob_pool * pool, const char * name)
{
    assert(identifier_table != NULL);
    assert(pool != NULL);
    assert(name != NULL);
    struct identifier * existing = hashmap_find(identifier_table, name);

    if (existing != NULL) {
        return existing;
    }

    return identifier_insert(identifier_table, pool, name, strlen(name));
}

struct identifier * identifier_lookup(struct hashmap * identifier_table, const char * name)
{
    assert(identifier_table != NULL);
    assert(name != NULL);
    return hashmap_find(identifier_table, name);
}

struct identifier * identifier_insert(struct hashmap * identifier_table, struct memory_blob_pool * pool, const char * name, unsigned int len)
{
    assert(identifier_table != NULL);
    assert(pool != NULL);
    assert(name != NULL);
    struct identifier * identifier = (struct identifier *) memory_blob_pool_alloc(pool, sizeof(struct identifier));
    memset(identifier, 0, sizeof(struct identifier));

    identifier->name = (char *) memory_blob_pool_alloc(pool, len + 1);
    strncpy(identifier->name, name, len);
    identifier->name[len] = '\0';

    hashmap_insert(identifier_table, identifier->name, identifier);
    return identifier;
}

void init_keywords(struct hashmap * identifier_table, struct memory_blob_pool * pool)
{
    assert(identifier_table != NULL);
    assert(pool != NULL);
    hashmap_init(identifier_table, IDENTIFIER_TABLE_SIZE, pool);

    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        const struct keyword * keyword = &keywords[i];

        struct identifier * identifier = identifier_create(identifier_table, pool, keyword->name);
        identifier->is_keyword = 1;
    }
}
