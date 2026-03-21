#include "cclynx.h"
#include "allocator.h"


void cclynx_init(struct cclynx_context * ctx)
{
    memory_blob_pool_init(&ctx->pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
    main_pool = &ctx->pool;
}

void cclynx_free(struct cclynx_context * ctx)
{
    memory_blob_pool_free(&ctx->pool, false);
    main_pool = NULL;
}
