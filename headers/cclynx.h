#ifndef CCLYNX_H
#define CCLYNX_H 1

#include "allocator.h"
#include "hashmap.h"
#include "scope.h"

struct cclynx_context {
    struct memory_blob_pool pool;
    struct hashmap identifier_table;
    struct scope global_scope;
};

void cclynx_init(struct cclynx_context * ctx);
void cclynx_free(struct cclynx_context * ctx);

#endif /* CCLYNX_H */
