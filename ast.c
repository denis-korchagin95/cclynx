#include <assert.h>
#include <memory.h>

#include "ast.h"
#include "allocator.h"
#include "type.h"
#include "error.h"

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

bool ast_statement_always_returns(const struct ast_node * node)
{
    if (node == NULL) {
        return false;
    }

    if (node->kind == AST_NODE_KIND_RETURN_STATEMENT) {
        return true;
    }

    if (node->kind == AST_NODE_KIND_IF_STATEMENT) {
        return node->content.if_statement.false_branch != NULL
            && ast_statement_always_returns(node->content.if_statement.true_branch)
            && ast_statement_always_returns(node->content.if_statement.false_branch);
    }

    if (node->kind == AST_NODE_KIND_COMPOUND_STATEMENT) {
        const struct ast_node_list * it = node->content.list;
        while (it != NULL) {
            if (ast_statement_always_returns(it->node)) {
                return true;
            }
            it = it->next;
        }
        return false;
    }

    return false;
}
