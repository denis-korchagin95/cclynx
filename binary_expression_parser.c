#include <assert.h>
#include <stddef.h>

#include "binary_expression_parser.h"
#include "parser.h"
#include "warning.h"
#include "tokenizer.h"
#include "type.h"

struct ast_node * parse_cast_expression(struct parser_context * ctx);

static const struct op_entry multiplicative_ops[] = {
    { TOKEN_KIND_PUNCTUATOR, '*', BINARY_OPERATION_MULTIPLY },
    { TOKEN_KIND_PUNCTUATOR, '/', BINARY_OPERATION_DIVIDE },
};

static const struct op_entry additive_ops[] = {
    { TOKEN_KIND_PUNCTUATOR, '+', BINARY_OPERATION_ADDITION },
    { TOKEN_KIND_PUNCTUATOR, '-', BINARY_OPERATION_SUBTRACTION },
};

static const struct op_entry relational_ops[] = {
    { TOKEN_KIND_PUNCTUATOR, '<', BINARY_OPERATION_LESS_THAN },
    { TOKEN_KIND_PUNCTUATOR, '>', BINARY_OPERATION_GREATER_THAN },
};

static const struct op_entry equality_ops[] = {
    { TOKEN_KIND_EQUAL_PUNCTUATOR,     0,   BINARY_OPERATION_EQUALITY },
    { TOKEN_KIND_NOT_EQUAL_PUNCTUATOR, 0,   BINARY_OPERATION_INEQUALITY },
};

static const struct binary_op_rule multiplicative_rule = {
    multiplicative_ops, 2, AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION, parse_cast_expression, WARNING_SIGN_CONVERSION
};

static const struct binary_op_rule additive_rule = {
    additive_ops, 2, AST_NODE_KIND_ADDITIVE_EXPRESSION, parse_multiplicative_expression, WARNING_SIGN_CONVERSION
};

static const struct binary_op_rule relational_rule = {
    relational_ops, 2, AST_NODE_KIND_RELATIONAL_EXPRESSION, parse_additive_expression, WARNING_SIGN_COMPARE
};

static const struct binary_op_rule equality_rule = {
    equality_ops, 2, AST_NODE_KIND_EQUALITY_EXPRESSION, parse_relational_expression, WARNING_SIGN_COMPARE
};

struct type * cast_binary_operands(struct parser_context * ctx, enum warning_code sign_warning, const struct token * op_token, struct ast_node ** lhs_ptr, struct ast_node ** rhs_ptr)
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

    return type_resolve(lhs_type, rhs_type);
}

static const struct op_entry * match_operator(const struct token * token, const struct binary_op_rule * rule)
{
    for (int i = 0; i < rule->num_operators; ++i) {
        const struct op_entry * entry = &rule->operators[i];
        if (
            token->kind == entry->token_kind
            && (entry->token_kind != TOKEN_KIND_PUNCTUATOR || token_first_ch(token) == entry->first_ch)
        ) {
            return entry;
        }
    }
    return NULL;
}

struct ast_node * parse_binary_expression(struct parser_context * ctx, const struct binary_op_rule * rule)
{
    assert(ctx != NULL);
    assert(rule != NULL);

    struct ast_node * lhs = rule->operand_parser(ctx);

    if (lhs == NULL) {
        return NULL;
    }

    struct token * current_token = parser_get_token(ctx);
    const struct op_entry * entry;

    while ((entry = match_operator(current_token, rule)) != NULL) {
        struct ast_node * rhs = rule->operand_parser(ctx);

        if (rhs == NULL) {
            return NULL;
        }

        struct ast_node * binary_expression = ast_create_node(ctx->pool, rule->node_kind, cast_binary_operands(ctx, rule->sign_warning, current_token, &lhs, &rhs));
        binary_expression->content.binary_expression.operation = entry->operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(ctx);
    }

    parser_putback_token(current_token, ctx);

    return lhs;
}

struct ast_node * parse_multiplicative_expression(struct parser_context * ctx)
{
    return parse_binary_expression(ctx, &multiplicative_rule);
}

struct ast_node * parse_additive_expression(struct parser_context * ctx)
{
    return parse_binary_expression(ctx, &additive_rule);
}

struct ast_node * parse_relational_expression(struct parser_context * ctx)
{
    return parse_binary_expression(ctx, &relational_rule);
}

struct ast_node * parse_equality_expression(struct parser_context * ctx)
{
    return parse_binary_expression(ctx, &equality_rule);
}
