#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#include "allocator.h"

static size_t align_up(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

struct memory_blob * alloc_new_blob(size_t size)
{
    struct memory_blob * blob = malloc(sizeof(struct memory_blob));
    memset(blob, 0, sizeof(struct memory_blob));
    blob->memory = malloc(size);
    blob->capacity = size;
    return blob;
}

struct memory_blob_pool * memory_blob_pool_create(size_t blob_size, size_t alignment)
{
    struct memory_blob_pool * pool = malloc(sizeof(struct memory_blob_pool));
    memset(pool, 0, sizeof(struct memory_blob_pool));
    pool->blob_size = blob_size;
    pool->alignment = alignment;
    pool->blob_capacity = DEFAULT_MEMORY_BLOB_CAPACITY;
    pool->blobs = malloc(sizeof(struct memory_blob *) * pool->blob_capacity);
    return pool;
}

void memory_blob_pool_init(struct memory_blob_pool * pool, size_t blob_size, size_t alignment)
{
    assert(pool != NULL);

    memset(pool, 0, sizeof(struct memory_blob_pool));
    pool->blob_size = blob_size;
    pool->alignment = alignment;
    pool->blob_capacity = DEFAULT_MEMORY_BLOB_CAPACITY;
    pool->blobs = malloc(sizeof(struct memory_blob *) * pool->blob_capacity);
}

void * memory_blob_pool_alloc(struct memory_blob_pool * pool, size_t size)
{
    assert(pool != NULL);

    size_t aligned_size = align_up(size, pool->alignment);

    for (size_t i = 0; i < pool->blob_count; ++i) {
        struct memory_blob * blob = pool->blobs[i];
        size_t aligned_offset = align_up(blob->used, pool->alignment);

        if (aligned_offset + aligned_size <= blob->capacity) {
            void * ptr = blob->memory + aligned_offset;
            blob->used += aligned_offset + aligned_size;
            return ptr;
        }
    }

    if (pool->blob_count >= pool->blob_capacity) {
        pool->blob_capacity *= 2;
        pool->blobs = realloc(pool->blobs, sizeof(struct memory_blob *) * pool->blob_capacity);
    }

    struct memory_blob * new_blob = alloc_new_blob(pool->blob_size);
    pool->blobs[pool->blob_count++] = new_blob;

    size_t aligned_offset = align_up(new_blob->used, pool->alignment);
    void * ptr = new_blob->memory + aligned_offset;
    new_blob->used += aligned_offset + aligned_size;

    return ptr;
}

void memory_blob_pool_free(struct memory_blob_pool * pool, bool free_pool)
{
    assert(pool != NULL);

    for (size_t i = 0; i < pool->blob_count; ++i) {
        free(pool->blobs[i]->memory);
        free(pool->blobs[i]);
    }

    free(pool->blobs);

    if (free_pool)
        free(pool);
}
