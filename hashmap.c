#include <assert.h>
#include <string.h>

#include "hashmap.h"
#include "allocator.h"


unsigned int hashmap_hash(const char * key, size_t len)
{
    assert(key != NULL);
    unsigned int hash = 0;
    for (size_t i = 0; i < len; ++i) {
        hash = hash * 31 + (unsigned int) key[i];
    }
    return hash;
}

void hashmap_init(struct hashmap * map, size_t capacity, struct memory_blob_pool * pool)
{
    assert(map != NULL);
    assert(pool != NULL);
    map->capacity = capacity;
    map->pool = pool;
    map->buckets = memory_blob_pool_alloc(pool, sizeof(struct hashmap_entry *) * capacity);
    memset(map->buckets, 0, sizeof(struct hashmap_entry *) * capacity);
}

void * hashmap_find(struct hashmap * map, const char * key, size_t len)
{
    assert(map != NULL);
    assert(key != NULL);
    unsigned int index = hashmap_hash(key, len) % map->capacity;

    for (struct hashmap_entry * it = map->buckets[index]; it != NULL; it = it->next) {
        if (strncmp(key, it->key, len) == 0 && it->key[len] == '\0') {
            return it->value;
        }
    }

    return NULL;
}

void hashmap_insert(struct hashmap * map, const char * key, void * value)
{
    assert(map != NULL);
    assert(key != NULL);
    unsigned int index = hashmap_hash(key, strlen(key)) % map->capacity;

    struct hashmap_entry * entry = memory_blob_pool_alloc(map->pool, sizeof(struct hashmap_entry));
    memset(entry, 0, sizeof(struct hashmap_entry));
    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
}
