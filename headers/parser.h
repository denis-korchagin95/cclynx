#ifndef PARSER_H
#define PARSER_H 1

#define MAX_TOKEN_BUFFER_SIZE (4)

enum ast_node_kind
{
    AST_NODE_KIND_INTEGER_CONSTANT = 1,
    AST_NODE_KIND_VARIABLE_DECLARATION,
    AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION,
    AST_NODE_KIND_ADDITIVE_EXPRESSION,
    AST_NODE_KIND_RELATIONAL_EXPRESSION,
    AST_NODE_KIND_EQUALITY_EXPRESSION,
};

enum binary_operation
{
    BINARY_OPERATION_MULTIPLY = 1,
    BINARY_OPERATION_DIVIDE,
    BINARY_OPERATION_ADDITION,
    BINARY_OPERATION_SUBTRACTION,
    BINARY_OPERATION_LESS_THAN,
    BINARY_OPERATION_GREATER_THAN,
    BINARY_OPERATION_EQUALITY,
    BINARY_OPERATION_INEQUALITY,
};

struct ast_node
{
    enum ast_node_kind kind;
    union
    {
        struct identifier * variable;
        long long int integer_constant;
        struct binary_expression
        {
            enum binary_operation operation;
            struct ast_node * lhs;
            struct ast_node * rhs;
        } binary_expression;
    } content;
};

struct parser_context
{
    struct token * tokens;
    struct token * iterator;
    struct token * token_buffer[MAX_TOKEN_BUFFER_SIZE];
    unsigned int token_buffer_pos;
};

void parser_init_context(struct parser_context * context, struct token * tokens);
struct ast_node * parser_parse(struct parser_context * context);

struct token * parser_get_token(struct parser_context * context);
void parser_putback_token(struct token * token, struct parser_context * context);

#endif /* PARSER_H */
