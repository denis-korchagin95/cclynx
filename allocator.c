#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#include "allocator.h"
#include "error.h"
#include "util.h"


struct memory_blob * alloc_new_blob(size_t size)
{
    struct memory_blob * blob = malloc(sizeof(struct memory_blob));
    if (blob == NULL) {
        cclynx_fatal_error("ERROR: failed to allocate memory blob\n");
    }
    memset(blob, 0, sizeof(struct memory_blob));
    blob->memory = malloc(size);
    if (blob->memory == NULL) {
        cclynx_fatal_error("ERROR: failed to allocate memory blob data (%zu bytes)\n", size);
    }
    blob->capacity = size;
    return blob;
}

struct memory_blob_pool * memory_blob_pool_create(size_t blob_size, size_t alignment)
{
    struct memory_blob_pool * pool = malloc(sizeof(struct memory_blob_pool));
    if (pool == NULL) {
        cclynx_fatal_error("ERROR: failed to allocate memory blob pool\n");
    }
    memset(pool, 0, sizeof(struct memory_blob_pool));
    pool->blob_size = blob_size;
    pool->alignment = alignment;
    pool->blob_capacity = DEFAULT_MEMORY_BLOB_CAPACITY;
    pool->blobs = malloc(sizeof(struct memory_blob *) * pool->blob_capacity);
    if (pool->blobs == NULL) {
        cclynx_fatal_error("ERROR: failed to allocate memory blob pool entries\n");
    }
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
    if (pool->blobs == NULL) {
        cclynx_fatal_error("ERROR: failed to allocate memory blob pool entries\n");
    }
}

void * memory_blob_pool_alloc(struct memory_blob_pool * pool, size_t size)
{
    assert(pool != NULL);

    size_t aligned_size = align_up(size, pool->alignment);

    if (pool->blob_count > 0) {
        struct memory_blob * blob = pool->blobs[pool->blob_count - 1];
        size_t aligned_offset = align_up(blob->used, pool->alignment);

        if (aligned_offset + aligned_size <= blob->capacity) {
            void * ptr = (char *)blob->memory + aligned_offset;
            blob->used = aligned_offset + aligned_size;
            return ptr;
        }
    }

    if (pool->blob_count >= pool->blob_capacity) {
        pool->blob_capacity *= 2;
        struct memory_blob ** new_blobs = realloc(pool->blobs, sizeof(struct memory_blob *) * pool->blob_capacity);
        if (new_blobs == NULL) {
            cclynx_fatal_error("ERROR: failed to reallocate memory blob pool entries\n");
        }
        pool->blobs = new_blobs;
    }

    if (aligned_size > pool->blob_size) {
        cclynx_fatal_error("ERROR: allocation of %zu bytes exceeds blob size of %zu bytes\n", size, pool->blob_size);
    }

    struct memory_blob * new_blob = alloc_new_blob(pool->blob_size);
    pool->blobs[pool->blob_count++] = new_blob;

    size_t aligned_offset = align_up(new_blob->used, pool->alignment);
    void * ptr = (char *)new_blob->memory + aligned_offset;
    new_blob->used = aligned_offset + aligned_size;

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
