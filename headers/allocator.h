#ifndef ALLOCATOR_H
#define ALLOCATOR_H 1

#include <stddef.h>
#include <stdbool.h>

#define DEFAULT_MEMORY_BLOB_SIZE (1024 * 1024)
#define DEFAULT_MEMORY_BLOB_ALIGNMENT (16)
#define DEFAULT_MEMORY_BLOB_CAPACITY (8)

struct memory_blob {
    void * memory;
    size_t used;
    size_t capacity;
};

struct memory_blob_pool {
    struct memory_blob ** blobs;
    size_t blob_count;
    size_t blob_capacity;
    size_t blob_size;
    size_t alignment;
};

struct memory_blob_pool * memory_blob_pool_create(size_t blob_size, size_t alignment);

void memory_blob_pool_init(struct memory_blob_pool * pool, size_t blob_size, size_t alignment);
void * memory_blob_pool_alloc(struct memory_blob_pool * pool, size_t size);
void memory_blob_pool_free(struct memory_blob_pool * pool, bool free_pool);

#endif /* ALLOCATOR_H */
