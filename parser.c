#include <assert.h>
#include <stddef.h>
#include <memory.h>
#include <stdlib.h>

#include "parser.h"

#include "tokenizer.h"
#include "allocator.h"
#include "identifier.h"

#include "print.h"
#include "symbol.h"

static struct ast_node * parse_expression(struct parser_context * context);
static struct ast_node * parse_jump_statement(struct parser_context * context);
static struct ast_node * parse_statement(struct parser_context * context);
static struct ast_node * parse_iteration_statement(struct parser_context * context);
static struct ast_node * parse_assignment_expression(struct parser_context * context);
static struct ast_node * parse_compound_statement(struct parser_context * context);
static struct ast_node * parse_expression_statement(struct parser_context * context);
static struct ast_node * parse_function_definition(struct parser_context * context);
static struct ast_node * parse_declaration(struct parser_context * context);
static struct ast_node * parse_equality_expression(struct parser_context * context);
static struct ast_node * parse_relational_expression(struct parser_context * context);
static struct ast_node * parse_additive_expression(struct parser_context * context);
static struct ast_node * parse_multiplicative_expression(struct parser_context * context);
static struct ast_node * parse_primary_expression(struct parser_context * context);

struct ast_node * parser_parse(struct parser_context * context)
{
    assert(context != NULL);

    return parse_function_definition(context);
}

struct ast_node * parse_statement(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    struct ast_node * statement = NULL;

    if (current_token->kind == TOKEN_KIND_IDENTIFIER && current_token->content.identifier->is_keyword) {
        parser_putback_token(current_token, context);

        if (strncmp("while", current_token->content.identifier->name, 5) == 0) {
            statement = parse_iteration_statement(context);
        } else if (strncmp("return", current_token->content.identifier->name, 6) == 0) {
            statement = parse_jump_statement(context);
        } else {
            statement = parse_declaration(context);
        }
    } else if (current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '{') {
        parser_putback_token(current_token, context);
        statement = parse_compound_statement(context);
    } else {
        parser_putback_token(current_token, context);
        statement = parse_expression_statement(context);
    }

    return statement;
}

struct ast_node * parse_compound_statement(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '{')) {
        fprintf(stderr, "ERROR: expected '{'!\n");
        exit(1);
    }

    struct ast_node_list * statement_list = NULL;
    struct ast_node_list ** statement_list_end = &statement_list;

    do {
        struct ast_node * statement = parse_statement(context);

        struct ast_node_list * list = (struct ast_node_list *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node_list));
        memset(list, 0, sizeof(struct ast_node_list));
        list->node = statement;
        list->next = NULL;

        *statement_list_end = list;
        statement_list_end = &list->next;

        current_token = parser_get_token(context);
        parser_putback_token(current_token, context);
    }
    while (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '}'));

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '}')) {
        fprintf(stderr, "ERROR: expected '}'!\n");
        exit(1);
    }

    (void)parser_get_token(context);

    struct ast_node * compound_statement = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(compound_statement, 0, sizeof(struct ast_node));
    compound_statement->kind = AST_NODE_KIND_COMPOUND_STATEMENT;
    compound_statement->content.list = statement_list;

    return compound_statement;
}

struct ast_node * parse_jump_statement(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (
        !(
        current_token->kind == TOKEN_KIND_IDENTIFIER
        && current_token->content.identifier->is_keyword
        && strncmp("return", current_token->content.identifier->name, 6) == 0
        )
    ) {
        fprintf(stderr, "ERROR: expected 'return'!\n");
        exit(1);
    }

    current_token = parser_get_token(context);

    struct ast_node * expression = NULL;

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ';')) {
        parser_putback_token(current_token, context);
        expression = parse_expression(context);
        current_token = parser_get_token(context);
    }

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ';')) {
        fprintf(stderr, "ERROR: expected ';'!\n");
        exit(1);
    }

    struct ast_node * jump_statement = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(jump_statement, 0, sizeof(struct ast_node));
    jump_statement->kind = AST_NODE_KIND_JUMP_STATEMENT;
    jump_statement->content.jump.type = JUMP_RETURN;
    jump_statement->content.jump.expression = expression;

    return jump_statement;
}

struct ast_node * parse_iteration_statement(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (
        !(current_token->kind == TOKEN_KIND_IDENTIFIER
        && current_token->content.identifier->is_keyword
        && strncmp("while", current_token->content.identifier->name, 5) == 0)
    ) {
        fprintf(stderr, "ERROR: expected 'while' keyword!\n");
        exit(1);
    }

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '(')) {
        fprintf(stderr, "ERROR: expected '('!\n");
        exit(1);
    }

    struct ast_node * expression = parse_expression(context);

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ')')) {
        fprintf(stderr, "ERROR: expected ')'!\n");
        exit(1);
    }

    struct ast_node * statement = parse_statement(context);

    struct ast_node * iteration_statement = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(iteration_statement, 0, sizeof(struct ast_node));
    iteration_statement->kind = AST_NODE_KIND_ITERATION_STATEMENT;
    iteration_statement->content.iteration.type = ITERATION_WHILE;
    iteration_statement->content.iteration.condition = expression;
    iteration_statement->content.iteration.body = statement;

    return iteration_statement;
}

struct ast_node * parse_expression_statement(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    struct ast_node * expression = NULL;

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ';')) {
        parser_putback_token(current_token, context);
        expression = parse_expression(context);
        current_token = parser_get_token(context);
    }

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ';')) {
        fprintf(stderr, "ERROR: expected ';'!\n");
        exit(1);
    }

    struct ast_node * expression_statement = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(expression_statement, 0, sizeof(struct ast_node));
    expression_statement->kind = AST_NODE_KIND_EXPRESSION_STATEMENT;
    expression_statement->content.node = expression;

    return expression_statement;
}

struct ast_node * parse_function_definition(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        fprintf(stderr, "ERROR: expected identifier!\n");
        exit(1);
    }

    const struct symbol * symbol = symbol_lookup(current_token->content.identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        fprintf(stderr, "ERROR: expected type specifier!\n");
        exit(1);
    }

    current_token = parser_get_token(context);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        fprintf(stderr, "ERROR: expected identifier!\n");
        exit(1);
    }

    if (current_token->content.identifier->is_keyword) {
        fprintf(stderr, "ERROR: expected identifier but not a keyword!\n");
        exit(1);
    }

    struct identifier * identifier = current_token->content.identifier;

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '(')) {
        fprintf(stderr, "ERROR: expected '('!\n");
        exit(1);
    }

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ')')) {
        fprintf(stderr, "ERROR: expected ')'!\n");
        exit(1);
    }

    struct ast_node * compound_statement = parse_compound_statement(context);

    struct ast_node * function_definition = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(function_definition, 0, sizeof(struct ast_node));
    function_definition->kind = AST_NODE_KIND_FUNCTION_DEFINITION;
    function_definition->content.function_definition.name = identifier;
    function_definition->content.function_definition.body = compound_statement;

    return function_definition;
}

struct ast_node * parse_declaration(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        fprintf(stderr, "ERROR: expected identifier!\n");
        exit(1);
    }

    const struct symbol * symbol = symbol_lookup(current_token->content.identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        fprintf(stderr, "ERROR: expected type specifier!\n");
        exit(1);
    }

    current_token = parser_get_token(context);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        fprintf(stderr, "ERROR: expected identifier!\n");
        exit(1);
    }

    if (current_token->content.identifier->is_keyword) {
        fprintf(stderr, "ERROR: expected identifier but not a keyword!\n");
        exit(1);
    }

    struct identifier * identifier = current_token->content.identifier;

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ';')) {
        fprintf(stderr, "ERROR: expected ';'!\n");
        exit(1);
    }

    struct ast_node * declaration = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(declaration, 0, sizeof(struct ast_node));
    declaration->kind = AST_NODE_KIND_VARIABLE_DECLARATION;
    declaration->content.variable = identifier;

    return declaration;
}

struct ast_node * parse_expression(struct parser_context * context)
{
    assert(context != NULL);

    return parse_assignment_expression(context);
}

struct ast_node * parse_assignment_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct ast_node * lhs = parse_equality_expression(context);

    if (lhs->kind != AST_NODE_KIND_VARIABLE) {
        return lhs;
    }

    struct token * current_token = parser_get_token(context);

    struct ast_node * initializer = NULL;

    if (current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '=') {
        initializer = parse_equality_expression(context);
    } else {
        parser_putback_token(current_token, context);
        return lhs;
    }

    struct ast_node * assignment_expression = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
    memset(assignment_expression, 0, sizeof(struct ast_node));
    assignment_expression->kind = AST_NODE_KIND_ASSIGNMENT_EXPRESSION;
    assignment_expression->content.assignment.type = ASSIGNMENT_REGULAR;
    assignment_expression->content.assignment.lhs = lhs;
    assignment_expression->content.assignment.initializer = initializer;

    return assignment_expression;
}

struct ast_node * parse_equality_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct ast_node * lhs = parse_relational_expression(context);

    struct token * current_token = parser_get_token(context);

    while (
        current_token->kind == TOKEN_KIND_EQUAL_PUNCTUATOR
        || current_token->kind == TOKEN_KIND_NOT_EQUAL_PUNCTUATOR
    ) {
        enum binary_operation operation = current_token->kind == TOKEN_KIND_EQUAL_PUNCTUATOR
            ? BINARY_OPERATION_EQUALITY
            : BINARY_OPERATION_INEQUALITY;
        struct ast_node * rhs = parse_multiplicative_expression(context);
        struct ast_node * binary_expression = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(binary_expression, 0, sizeof(struct ast_node));
        binary_expression->kind = AST_NODE_KIND_EQUALITY_EXPRESSION;
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(context);
    }

    parser_putback_token(current_token, context);

    return lhs;
}

struct ast_node * parse_relational_expression(struct parser_context * context) {
    assert(context != NULL);

    struct ast_node * lhs = parse_additive_expression(context);

    struct token * current_token = parser_get_token(context);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            current_token->content.ch == '<'
            || current_token->content.ch == '>'
        )
    ) {
        enum binary_operation operation = current_token->content.ch == '<'
            ? BINARY_OPERATION_LESS_THAN
            : BINARY_OPERATION_GREATER_THAN;
        struct ast_node * rhs = parse_multiplicative_expression(context);
        struct ast_node * binary_expression = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(binary_expression, 0, sizeof(struct ast_node));
        binary_expression->kind = AST_NODE_KIND_RELATIONAL_EXPRESSION;
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(context);
    }

    parser_putback_token(current_token, context);

    return lhs;
}


struct ast_node * parse_additive_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct ast_node * lhs = parse_multiplicative_expression(context);

    struct token * current_token = parser_get_token(context);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            current_token->content.ch == '+'
            || current_token->content.ch == '-'
        )
    ) {
        enum binary_operation operation = current_token->content.ch == '+'
            ? BINARY_OPERATION_ADDITION
            : BINARY_OPERATION_SUBTRACTION;
        struct ast_node * rhs = parse_multiplicative_expression(context);
        struct ast_node * binary_expression = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(binary_expression, 0, sizeof(struct ast_node));
        binary_expression->kind = AST_NODE_KIND_ADDITIVE_EXPRESSION;
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(context);
    }

    parser_putback_token(current_token, context);

    return lhs;
}

struct ast_node * parse_multiplicative_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct ast_node * lhs = parse_primary_expression(context);

    struct token * current_token = parser_get_token(context);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            current_token->content.ch == '*'
            || current_token->content.ch == '/'
        )
    ) {
        enum binary_operation operation = current_token->content.ch == '*'
            ? BINARY_OPERATION_MULTIPLY
            : BINARY_OPERATION_DIVIDE;
        struct ast_node * rhs = parse_primary_expression(context);
        struct ast_node * binary_expression = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(binary_expression, 0, sizeof(struct ast_node));
        binary_expression->kind = AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION;
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(context);
    }

    parser_putback_token(current_token, context);

    return lhs;
}

struct ast_node * parse_primary_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (current_token->kind == TOKEN_KIND_NUMBER) {
        struct ast_node * number = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(number, 0, sizeof(struct ast_node));
        number->kind = AST_NODE_KIND_INTEGER_CONSTANT;
        number->content.integer_constant = current_token->content.integer_constant;
        return number;
    }

    if (current_token->kind == TOKEN_KIND_IDENTIFIER) {
        struct ast_node * variable = (struct ast_node *) memory_blob_pool_alloc(&main_pool, sizeof(struct ast_node));
        memset(variable, 0, sizeof(struct ast_node));
        variable->kind = AST_NODE_KIND_VARIABLE;
        variable->content.variable = current_token->content.identifier;
        return variable;
    }

    if (current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '(') {
        struct ast_node * expression = parse_expression(context);

        current_token = parser_get_token(context);

        if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ')')) {
            fprintf(stderr, "ERROR: expected ')'!\n");
            exit(1);
        }

        return expression;
    }

    fprintf(stderr, "ERROR: expected number or variable!\n");
    exit(1);
}

struct token * parser_get_token(struct parser_context * context)
{
    assert(context != NULL);

    if (context->token_buffer_pos > 0)
        return context->token_buffer[--context->token_buffer_pos];

    struct token * current_token = context->iterator;
    context->iterator = context->iterator->next;

    return current_token;
}

void parser_putback_token(struct token * token, struct parser_context * context)
{
    assert(token != NULL);
    assert(context != NULL);

    if (token == &eos_token) {
        fprintf(stderr, "ERROR: Trying to putback EOS token!\n");
        exit(1);
    }

    if (context->token_buffer_pos >= MAX_TOKEN_BUFFER_SIZE) {
        fprintf(stderr, "ERROR: Maximum token buffer size reached!\n");
        exit(1);
    }

    context->token_buffer[context->token_buffer_pos++] = token;
}

void parser_init_context(struct parser_context * context, struct token * tokens)
{
    assert(context != NULL);
    assert(tokens != NULL);

    memset(context, 0, sizeof(struct parser_context));
    context->tokens = tokens;
    context->iterator = context->tokens;
}
