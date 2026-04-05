#ifndef CCLYNX_BINARY_EXPRESSION_PARSER_H
#define CCLYNX_BINARY_EXPRESSION_PARSER_H 1

#include "ast.h"
#include "warning.h"

struct parser_context;
struct token;

typedef struct ast_node * (*parse_fn)(struct parser_context *);

struct op_entry {
    int token_kind;
    char first_ch;
    enum binary_operation operation;
};

struct binary_op_rule {
    const struct op_entry * operators;
    int num_operators;
    enum ast_node_kind node_kind;
    parse_fn operand_parser;
    enum warning_code sign_warning;
};

struct ast_node * parse_binary_expression(struct parser_context * ctx, const struct binary_op_rule * rule);

struct ast_node * parse_equality_expression(struct parser_context * ctx);
struct ast_node * parse_relational_expression(struct parser_context * ctx);
struct ast_node * parse_additive_expression(struct parser_context * ctx);
struct ast_node * parse_multiplicative_expression(struct parser_context * ctx);

#endif /* CCLYNX_BINARY_EXPRESSION_PARSER_H */
