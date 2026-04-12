#include <assert.h>
#include <stddef.h>

#include "ast.h"
#include "binary_expression_parser.h"
#include "parser.h"
#include "warning.h"
#include "tokenizer.h"
#include "type.h"

struct ast_node * parse_cast_expression(struct parser_context * ctx);

struct op_entry {
    int token_kind;
    char first_ch;
    enum binary_operation operation;
    enum binary_expression_precedence precedence;
    enum ast_node_kind node_kind;
    enum warning_code sign_warning;
};

static const struct op_entry ops[] = {
    { TOKEN_KIND_EQUAL_PUNCTUATOR,     0,   BINARY_OPERATION_EQUALITY,     BINARY_EXPRESSION_PRECEDENCE_EQUALITY,       AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_COMPARE },
    { TOKEN_KIND_NOT_EQUAL_PUNCTUATOR, 0,   BINARY_OPERATION_INEQUALITY,   BINARY_EXPRESSION_PRECEDENCE_EQUALITY,       AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_COMPARE },
    { TOKEN_KIND_PUNCTUATOR,           '<', BINARY_OPERATION_LESS_THAN,    BINARY_EXPRESSION_PRECEDENCE_RELATIONAL,     AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_COMPARE },
    { TOKEN_KIND_PUNCTUATOR,           '>', BINARY_OPERATION_GREATER_THAN, BINARY_EXPRESSION_PRECEDENCE_RELATIONAL,     AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_COMPARE },
    { TOKEN_KIND_PUNCTUATOR,           '+', BINARY_OPERATION_ADDITION,     BINARY_EXPRESSION_PRECEDENCE_ADDITIVE,       AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_CONVERSION },
    { TOKEN_KIND_PUNCTUATOR,           '-', BINARY_OPERATION_SUBTRACTION,  BINARY_EXPRESSION_PRECEDENCE_ADDITIVE,       AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_CONVERSION },
    { TOKEN_KIND_PUNCTUATOR,           '*', BINARY_OPERATION_MULTIPLY,     BINARY_EXPRESSION_PRECEDENCE_MULTIPLICATIVE, AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_CONVERSION },
    { TOKEN_KIND_PUNCTUATOR,           '/', BINARY_OPERATION_DIVIDE,       BINARY_EXPRESSION_PRECEDENCE_MULTIPLICATIVE, AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_CONVERSION },
    { TOKEN_KIND_PUNCTUATOR,           '%', BINARY_OPERATION_MODULO,       BINARY_EXPRESSION_PRECEDENCE_MULTIPLICATIVE, AST_NODE_KIND_BINARY_EXPRESSION, WARNING_SIGN_CONVERSION },
};

static const int num_ops = sizeof(ops) / sizeof(ops[0]);

static struct type * cast_binary_operands(struct parser_context * ctx, enum warning_code sign_warning, const struct token * op_token, struct ast_node ** lhs_ptr, struct ast_node ** rhs_ptr)
{
    assert(ctx != NULL);
    assert(op_token != NULL);
    assert(lhs_ptr != NULL);
    assert(rhs_ptr != NULL);

    struct type * lhs_type = (*lhs_ptr)->type;
    struct type * rhs_type = (*rhs_ptr)->type;

    if (lhs_type->kind == rhs_type->kind && type_signedness_differs(lhs_type, rhs_type)) {
        parser_report_warning(ctx, sign_warning, op_token,
            "implicit conversion changes signedness from '%s' to '%s', use an explicit cast",
            type_stringify(type_is_unsigned(lhs_type) ? rhs_type : lhs_type),
            type_stringify(type_is_unsigned(lhs_type) ? lhs_type : rhs_type));
        struct type * target_type = type_is_unsigned(lhs_type) ? lhs_type : rhs_type;

        if (!type_is_unsigned(lhs_type)) {
            struct ast_node * cast = ast_create_node(ctx->pool, AST_NODE_KIND_CAST_EXPRESSION, target_type);
            cast->content.node = *lhs_ptr;
            *lhs_ptr = cast;
        } else {
            struct ast_node * cast = ast_create_node(ctx->pool, AST_NODE_KIND_CAST_EXPRESSION, target_type);
            cast->content.node = *rhs_ptr;
            *rhs_ptr = cast;
        }

        return target_type;
    }

    return type_check(lhs_type, rhs_type);
}

static const struct op_entry * match_operator(const struct token * token, enum binary_expression_precedence min_precedence)
{
    for (int i = 0; i < num_ops; ++i) {
        const struct op_entry * entry = &ops[i];
        if (
            entry->precedence >= min_precedence
            && token->kind == entry->token_kind
            && (entry->token_kind != TOKEN_KIND_PUNCTUATOR || token_first_ch(token) == entry->first_ch)
        ) {
            return entry;
        }
    }
    return NULL;
}

struct ast_node * parse_binary_expression(struct parser_context * ctx, enum binary_expression_precedence min_precedence)
{
    assert(ctx != NULL);

    struct ast_node * lhs = parse_cast_expression(ctx);

    if (lhs == NULL) {
        return NULL;
    }

    const struct op_entry * entry;

    while ((entry = match_operator(parser_peek_token(ctx), min_precedence)) != NULL) {
        struct token * current_token = parser_get_token(ctx);
        assert(entry->precedence + 1 <= BINARY_EXPRESSION_PRECEDENCE_NONE);
        struct ast_node * rhs = parse_binary_expression(ctx, entry->precedence + 1);

        if (rhs == NULL) {
            return NULL;
        }

        struct ast_node * binary_expression = ast_create_node(ctx->pool, entry->node_kind, cast_binary_operands(ctx, entry->sign_warning, current_token, &lhs, &rhs));
        binary_expression->content.binary_expression.operation = entry->operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;
    }

    return lhs;
}
