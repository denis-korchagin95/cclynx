#ifndef PARSER_H
#define PARSER_H 1

enum ast_node_kind
{
    AST_NODE_KIND_IDENTIFIER = 1,
    AST_NODE_KIND_INTEGER_CONSTANT,
};

struct ast_node
{
    enum ast_node_kind kind;
    union
    {
        struct variable
        {
            struct identifier * identifier;
            struct symbol * symbol;
        } variable;
        long long int integer_constant;
    } content;
};

struct parser_context
{
    struct token * tokens;
    struct token * next_token;
};

struct ast_node * parser_parse(struct parser_context * context);

#endif /* PARSER_H */
