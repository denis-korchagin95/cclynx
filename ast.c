#include <assert.h>
#include <memory.h>

#include "ast.h"
#include "allocator.h"
#include "type.h"
#include "errors.h"

struct ast_node * ast_create_node(struct memory_blob_pool * pool, enum ast_node_kind kind, struct type * type)
{
    assert(pool != NULL);
    assert(type != NULL);
    struct ast_node * node = memory_blob_pool_alloc(pool, sizeof(struct ast_node));
    memset(node, 0, sizeof(struct ast_node));
    node->kind = kind;
    node->type = type;
    return node;
}
