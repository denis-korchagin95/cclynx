#ifndef PARSER_H
#define PARSER_H 1

#define MAX_TOKEN_BUFFER_SIZE (4)

enum ast_node_kind
{
    AST_NODE_KIND_INTEGER_CONSTANT = 1,
    AST_NODE_KIND_VARIABLE_DECLARATION,
    AST_NODE_KIND_VARIABLE,
    AST_NODE_KIND_FUNCTION_DEFINITION,
    AST_NODE_KIND_COMPOUND_STATEMENT,
    AST_NODE_KIND_EXPRESSION_STATEMENT,
    AST_NODE_KIND_ITERATION_STATEMENT,
    AST_NODE_KIND_JUMP_STATEMENT,
    AST_NODE_KIND_ASSIGNMENT_EXPRESSION,
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

struct ast_node_list {
    struct ast_node * node;
    struct ast_node_list * next;
};

enum assignment_type {
    ASSIGNMENT_REGULAR = 1,
};

enum iteration_type {
    ITERATION_WHILE = 1,
};

enum jump_type {
    JUMP_RETURN = 1,
};

struct ast_node
{
    union
    {
        struct identifier * variable;
        long long int integer_constant;
        struct ast_node_list * list;
        struct ast_node * node;
        struct binary_expression
        {
            enum binary_operation operation;
            struct ast_node * lhs;
            struct ast_node * rhs;
        } binary_expression;
        struct function_definition
        {
            struct identifier * name;
            struct ast_node * body;
        } function_definition;
        struct assignment {
            enum assignment_type type;
            struct ast_node * lhs;
            struct ast_node * initializer;
        } assignment;
        struct iteration {
            enum iteration_type type;
            struct ast_node * condition;
            struct ast_node * body;
        } iteration;
        struct jump {
            enum jump_type type;
            struct ast_node * expression;
        } jump;
    } content;
    enum ast_node_kind kind;
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
