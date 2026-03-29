#ifndef CCLYNX_AST_H
#define CCLYNX_AST_H 1

#define MAX_AST_FUNCTION_PARAMETER_COUNT (3)
#define MAX_AST_FUNCTION_ARGUMENT_COUNT (3)

struct symbol;
struct identifier;

enum ast_node_kind
{
    AST_NODE_KIND_TRANSLATION_UNIT = 1,
    AST_NODE_KIND_INTEGER_CONSTANT,
    AST_NODE_KIND_FLOAT_CONSTANT,
    AST_NODE_KIND_VARIABLE,
    AST_NODE_KIND_FUNCTION_CALL,

    AST_NODE_KIND_VARIABLE_DECLARATION,
    AST_NODE_KIND_FUNCTION_PARAMETER,
    AST_NODE_KIND_FUNCTION_DEFINITION,

    AST_NODE_KIND_COMPOUND_STATEMENT,
    AST_NODE_KIND_EXPRESSION_STATEMENT,
    AST_NODE_KIND_WHILE_STATEMENT,
    AST_NODE_KIND_RETURN_STATEMENT,
    AST_NODE_KIND_IF_STATEMENT,

    AST_NODE_KIND_CAST_EXPRESSION,
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

struct ast_node
{
    union
    {
        struct symbol * symbol;
        struct ast_node_list * list;
        struct translation_unit {
            struct ast_node_list * list;
            const char * filename;
        } translation_unit;
        struct ast_node * node;
        struct constant
        {
            union {
                long long int integer_constant;
                float float_constant;
            } value;
        } constant;
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
            enum {
                PARAMETER_PRESENCE_UNSPECIFIED = 0,
                PARAMETER_PRESENCE_VOID,
                PARAMETER_PRESENCE_SPECIFIED,
            } parameter_presence;
            struct ast_node * parameters[MAX_AST_FUNCTION_PARAMETER_COUNT];
            unsigned int parameter_count;
        } function_definition;
        struct function_call
        {
        struct symbol * function;
            struct ast_node * arguments[MAX_AST_FUNCTION_ARGUMENT_COUNT];
            unsigned int argument_count;
        } function_call;
        struct assignment {
            enum assignment_type type;
            struct ast_node * lhs;
            struct ast_node * initializer;
        } assignment;
        struct while_statement {
            struct ast_node * condition;
            struct ast_node * body;
        } while_statement;
        struct if_statement {
            struct ast_node * condition;
            struct ast_node * true_branch;
            struct ast_node * false_branch;
        } if_statement;
    } content;
    struct type * type;
    enum ast_node_kind kind;
};

struct memory_blob_pool;

struct ast_node * ast_create_node(struct memory_blob_pool * pool, enum ast_node_kind kind, struct type * type);

#endif /* CCLYNX_AST_H */
