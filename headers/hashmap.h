#ifndef CCLYNX_HASHMAP_H
#define CCLYNX_HASHMAP_H 1

#include <stddef.h>

struct memory_blob_pool;

struct hashmap_entry
{
    const char * key;
    void * value;
    struct hashmap_entry * next;
};

struct hashmap
{
    struct hashmap_entry ** buckets;
    size_t capacity;
    struct memory_blob_pool * pool;
};

unsigned int hashmap_hash(const char * key, size_t len);

void hashmap_init(struct hashmap * map, size_t capacity, struct memory_blob_pool * pool);
void * hashmap_find(struct hashmap * map, const char * key, size_t len);
void hashmap_insert(struct hashmap * map, const char * key, void * value);

#endif /* CCLYNX_HASHMAP_H */
