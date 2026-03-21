#include <assert.h>
#include <memory.h>

#include "cclynx.h"
#include "allocator.h"


void cclynx_init(struct cclynx_context * ctx)
{
    assert(ctx != NULL);
    memset(ctx, 0, sizeof(struct cclynx_context));
    memory_blob_pool_init(&ctx->pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
}

void cclynx_free(struct cclynx_context * ctx)
{
    assert(ctx != NULL);
    memory_blob_pool_free(&ctx->pool, false);
}
