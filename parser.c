#include <assert.h>
#include <stddef.h>
#include <memory.h>

#include "parser.h"
#include "tokenizer.h"
#include "allocator.h"

struct ast_node * parser_parse(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = context->next_token;

    if (current_token->kind == TOKEN_KIND_NUMBER) {
        struct ast_node * number = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(number, 0, sizeof(struct ast_node));
        number->kind = AST_NODE_KIND_INTEGER_CONSTANT;
        number->content.integer_constant = current_token->content.integer_constant;
        context->next_token = current_token->next;
        return number;
    }

    return NULL;
}