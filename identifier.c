#include <string.h>

#include "identifier.h"
#include "allocator.h"

#define IDENTIFIER_TABLE_SIZE  (101)

static struct identifier * identifiers[IDENTIFIER_TABLE_SIZE] = {0};

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
    unsigned int hash = get_string_hash(name);
    unsigned int len = strlen(name);
    return identifier_insert(hash, name, len);
}

struct identifier * identifier_lookup(unsigned int hash, const char * name)
{
    unsigned int index = hash % IDENTIFIER_TABLE_SIZE;
    struct identifier * it;
    for(it = identifiers[index]; it != NULL; it = it->next) {
        if (strcmp(name, it->name) == 0) {
            return it;
        }
    }
    return NULL;
}

struct identifier * identifier_insert(unsigned int hash, const char * name, unsigned int len)
{
    unsigned int index = hash % IDENTIFIER_TABLE_SIZE;

    struct identifier * identifier = memory_blob_pool_alloc(&main_pool, sizeof(struct identifier));

    identifier->name = (char *) memory_blob_pool_alloc(&main_pool, len + 1);
    strncpy(identifier->name, name, len);
    identifier->name[len] = '\0';

    identifier->next = identifiers[index];
    identifiers[index] = identifier;
    return identifier;
}
