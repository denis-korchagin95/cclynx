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
#include "type.h"

static struct ast_node * create_ast_node(enum ast_node_kind kind);

static struct ast_node * parse_expression(struct parser_context * context);
static struct ast_node * parse_if_statement(struct parser_context * context);
static struct ast_node * parse_return_statement(struct parser_context * context);
static struct ast_node * parse_statement(struct parser_context * context);
static struct ast_node * parse_while_statement(struct parser_context * context);
static struct ast_node * parse_assignment_expression(struct parser_context * context);
static struct ast_node * parse_compound_statement(struct parser_context * context);
static struct ast_node * parse_expression_statement(struct parser_context * context);
static struct ast_node * parse_function_definition(struct parser_context * context);
static struct ast_node * parse_declaration(struct parser_context * context);
static struct ast_node * parse_equality_expression(struct parser_context * context);
static struct ast_node * parse_relational_expression(struct parser_context * context);
static struct ast_node * parse_additive_expression(struct parser_context * context);
static struct ast_node * parse_multiplicative_expression(struct parser_context * context);
static struct ast_node * parse_cast_expression(struct parser_context * context);
static struct ast_node * parse_primary_expression(struct parser_context * context);

static struct type * check_type(struct type * lhs, struct type * rhs);

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
        if (strncmp("while", current_token->content.identifier->name, 5) == 0) {
            statement = parse_while_statement(context);
        } else if (strncmp("return", current_token->content.identifier->name, 6) == 0) {
            statement = parse_return_statement(context);
        } else if (strncmp("if", current_token->content.identifier->name, 2) == 0) {
            statement = parse_if_statement(context);
        } else {
            parser_putback_token(current_token, context);
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

    current_token = parser_get_token(context);

    if (current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '}') {
        struct ast_node * compound_statement = create_ast_node(AST_NODE_KIND_COMPOUND_STATEMENT);
        compound_statement->content.list = NULL;
        return compound_statement;
    }

    parser_putback_token(current_token, context);

    struct ast_node_list * statement_list = NULL;
    struct ast_node_list ** statement_list_end = &statement_list;

    do {
        struct ast_node * statement = parse_statement(context);

        main_pool_alloc(struct ast_node_list, list);
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


    struct ast_node * compound_statement = create_ast_node(AST_NODE_KIND_COMPOUND_STATEMENT);
    compound_statement->content.list = statement_list;

    return compound_statement;
}

struct ast_node * parse_if_statement(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '(')) {
        fprintf(stderr, "ERROR: expected '('!\n");
        exit(1);
    }

    struct ast_node * condition = parse_expression(context);

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ')')) {
        fprintf(stderr, "ERROR: expected ')'!\n");
        exit(1);
    }

    struct ast_node * true_branch = parse_statement(context);
    struct ast_node * false_branch = NULL;

    current_token = parser_get_token(context);

    if (
        current_token->kind == TOKEN_KIND_IDENTIFIER
        && current_token->content.identifier->is_keyword
        && strncmp("else", current_token->content.identifier->name, 4) == 0
    ) {
        false_branch = parse_statement(context);
    } else {
        parser_putback_token(current_token, context);
    }

    struct ast_node * if_statement = create_ast_node(AST_NODE_KIND_IF_STATEMENT);
    if_statement->content.if_statement.condition = condition;
    if_statement->content.if_statement.true_branch = true_branch;
    if_statement->content.if_statement.false_branch = false_branch;

    return if_statement;
}

struct ast_node * parse_return_statement(struct parser_context * context)
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

    struct ast_node * return_statement = create_ast_node(AST_NODE_KIND_RETURN_STATEMENT);
    return_statement->content.node = expression;

    return return_statement;
}

struct ast_node * parse_while_statement(struct parser_context * context)
{
    assert(context != NULL);

    const struct token * current_token = parser_get_token(context);

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

    struct ast_node * while_statement = create_ast_node(AST_NODE_KIND_WHILE_STATEMENT);
    while_statement->content.while_statement.condition = expression;
    while_statement->content.while_statement.body = statement;

    return while_statement;
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

    struct ast_node * expression_statement = create_ast_node(AST_NODE_KIND_EXPRESSION_STATEMENT);
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

    struct ast_node * function_definition = create_ast_node(AST_NODE_KIND_FUNCTION_DEFINITION);
    function_definition->content.function_definition.name = identifier;
    function_definition->content.function_definition.body = compound_statement;
    function_definition->type = symbol->type;

    return function_definition;
}

struct ast_node * parse_declaration(struct parser_context * context)
{
    assert(context != NULL);

    const struct token * current_token = parser_get_token(context);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        fprintf(stderr, "ERROR: expected identifier!\n");
        exit(1);
    }

    const struct symbol * symbol = symbol_lookup(current_token->content.identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        fprintf(stderr, "ERROR: expected type specifier!\n");
        exit(1);
    }

    struct type * type = symbol->type;

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

    symbol = symbol_lookup(identifier, SYMBOL_KIND_VARIABLE);

    if (symbol != NULL) {
        fprintf(stderr, "ERROR: variable '%s' already declared!\n", identifier->name);
        exit(1);
    }

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ';')) {
        fprintf(stderr, "ERROR: expected ';'!\n");
        exit(1);
    }

    main_pool_alloc(struct symbol, variable)
    variable->kind = SYMBOL_KIND_VARIABLE;
    variable->identifier = identifier;
    variable->type = type;

    identifier_attach_symbol(identifier, variable)

    struct ast_node * declaration = create_ast_node(AST_NODE_KIND_VARIABLE_DECLARATION);
    declaration->content.variable = variable;
    declaration->type = variable->type;

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

    struct ast_node * assignment_expression = create_ast_node(AST_NODE_KIND_ASSIGNMENT_EXPRESSION);
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
        const enum binary_operation operation = current_token->kind == TOKEN_KIND_EQUAL_PUNCTUATOR
            ? BINARY_OPERATION_EQUALITY
            : BINARY_OPERATION_INEQUALITY;
        struct ast_node * rhs = parse_multiplicative_expression(context);

        struct ast_node * binary_expression = create_ast_node(AST_NODE_KIND_EQUALITY_EXPRESSION);
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        binary_expression->type = check_type(lhs->type, rhs->type);
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
        const enum binary_operation operation = current_token->content.ch == '<'
            ? BINARY_OPERATION_LESS_THAN
            : BINARY_OPERATION_GREATER_THAN;
        struct ast_node * rhs = parse_multiplicative_expression(context);

        struct ast_node * binary_expression = create_ast_node(AST_NODE_KIND_RELATIONAL_EXPRESSION);
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        binary_expression->type = check_type(lhs->type, rhs->type);
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
        const enum binary_operation operation = current_token->content.ch == '+'
            ? BINARY_OPERATION_ADDITION
            : BINARY_OPERATION_SUBTRACTION;
        struct ast_node * rhs = parse_multiplicative_expression(context);

        struct ast_node * binary_expression = create_ast_node(AST_NODE_KIND_ADDITIVE_EXPRESSION);
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        binary_expression->type = check_type(lhs->type, rhs->type);
        lhs = binary_expression;

        current_token = parser_get_token(context);
    }

    parser_putback_token(current_token, context);

    return lhs;
}

struct ast_node * parse_multiplicative_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct ast_node * lhs = parse_cast_expression(context);

    struct token * current_token = parser_get_token(context);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            current_token->content.ch == '*'
            || current_token->content.ch == '/'
        )
    ) {
        const enum binary_operation operation = current_token->content.ch == '*'
            ? BINARY_OPERATION_MULTIPLY
            : BINARY_OPERATION_DIVIDE;
        struct ast_node * rhs = parse_cast_expression(context);

        struct ast_node * binary_expression = create_ast_node(AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION);
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        binary_expression->type = check_type(lhs->type, rhs->type);
        lhs = binary_expression;

        current_token = parser_get_token(context);
    }

    parser_putback_token(current_token, context);

    return lhs;
}

struct ast_node * parse_cast_expression(struct parser_context * context)
{
    assert(context != NULL);

    struct token * current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == '(')) {
        parser_putback_token(current_token, context);
        return parse_primary_expression(context);
    }

    struct token * previous_token = current_token;

    current_token = parser_get_token(context);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        parser_putback_token(current_token, context);
        parser_putback_token(previous_token, context);
        return parse_primary_expression(context);
    }

    const struct symbol * symbol = symbol_lookup(current_token->content.identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        parser_putback_token(current_token, context);
        parser_putback_token(previous_token, context);
        return parse_primary_expression(context);
    }

    struct type * type = symbol->type;

    current_token = parser_get_token(context);

    if (!(current_token->kind == TOKEN_KIND_PUNCTUATOR && current_token->content.ch == ')')) {
        fprintf(stderr, "ERROR: expected ')'!\n");
        exit(1);
    }

    struct ast_node * expression = parse_primary_expression(context);

    struct ast_node * cast_expression = create_ast_node(AST_NODE_KIND_CAST_EXPRESSION);
    cast_expression->type = type;
    cast_expression->content.node = expression;

    return cast_expression;
}

struct ast_node * parse_primary_expression(struct parser_context * context)
{
    assert(context != NULL);

    const struct token * current_token = parser_get_token(context);

    if (current_token->kind == TOKEN_KIND_NUMBER) {
        if ((current_token->flags & TOKEN_FLAG_IS_FLOAT) > 0) {
            struct ast_node * number = create_ast_node(AST_NODE_KIND_FLOAT_CONSTANT);
            number->content.constant.value.float_constant = (float)atof(current_token->content.number);
            number->type = &type_float;
            return number;
        }

        struct ast_node * number = create_ast_node(AST_NODE_KIND_INTEGER_CONSTANT);
        number->content.constant.value.integer_constant = atoi(current_token->content.number);
        number->type = &type_integer;
        return number;
    }

    if (current_token->kind == TOKEN_KIND_IDENTIFIER) {
        if (current_token->content.identifier->is_keyword) {
            fprintf(stderr, "ERROR: expected identifier but not a keyword!\n");
            exit(1);
        }

        struct symbol * symbol = symbol_lookup(current_token->content.identifier, SYMBOL_KIND_VARIABLE);

        if (symbol == NULL) {
            fprintf(stderr, "ERROR: undeclared variable \"%s\"!\n", current_token->content.identifier->name);
            exit(1);
        }

        struct ast_node * variable = create_ast_node(AST_NODE_KIND_VARIABLE);
        variable->content.variable = symbol;
        variable->type = symbol->type;
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

struct ast_node * create_ast_node(enum ast_node_kind kind)
{
    main_pool_alloc(struct ast_node, node);
    node->kind = kind;
    node->type = &type_void;
    return node;
}

struct type * check_type(struct type * lhs, struct type * rhs)
{
    assert(lhs != NULL);
    assert(rhs != NULL);

    if (lhs->kind != rhs->kind) {
        fprintf(stderr, "ERROR: type mismatch between '%s' and '%s'\n", type_stringify(lhs), type_stringify(rhs));
        exit(1);
    }

    return lhs;
}
