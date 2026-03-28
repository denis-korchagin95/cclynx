#include <assert.h>
#include <string.h>

#include "hashmap.h"
#include "allocator.h"


unsigned int hashmap_hash(const char * key)
{
    assert(key != NULL);
    unsigned int hash = 0;
    while (*key != '\0') {
        hash = hash * 31 + (unsigned int) *key;
        ++key;
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

void * hashmap_find(struct hashmap * map, const char * key)
{
    assert(map != NULL);
    assert(key != NULL);
    unsigned int index = hashmap_hash(key) % map->capacity;

    for (struct hashmap_entry * it = map->buckets[index]; it != NULL; it = it->next) {
        if (strcmp(key, it->key) == 0) {
            return it->value;
        }
    }

    return NULL;
}

void hashmap_insert(struct hashmap * map, const char * key, void * value)
{
    assert(map != NULL);
    assert(key != NULL);
    unsigned int index = hashmap_hash(key) % map->capacity;

    struct hashmap_entry * entry = memory_blob_pool_alloc(map->pool, sizeof(struct hashmap_entry));
    memset(entry, 0, sizeof(struct hashmap_entry));
    entry->key = key;
    entry->value = value;
    entry->next = map->buckets[index];
    map->buckets[index] = entry;
}
