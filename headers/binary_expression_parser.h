#ifndef CCLYNX_BINARY_EXPRESSION_PARSER_H
#define CCLYNX_BINARY_EXPRESSION_PARSER_H 1

struct parser_context;

enum binary_expression_precedence {
    BINARY_EXPRESSION_PRECEDENCE_EQUALITY = 1,
    BINARY_EXPRESSION_PRECEDENCE_RELATIONAL,
    BINARY_EXPRESSION_PRECEDENCE_ADDITIVE,
    BINARY_EXPRESSION_PRECEDENCE_MULTIPLICATIVE,
    BINARY_EXPRESSION_PRECEDENCE_NONE,
};

struct ast_node * parse_binary_expression(struct parser_context * ctx, enum binary_expression_precedence min_precedence);

#endif /* CCLYNX_BINARY_EXPRESSION_PARSER_H */
