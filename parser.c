#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "tokenizer.h"
#include "allocator.h"
#include "identifier.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"
#include "errors.h"


static struct ast_node * parse_translation_unit(struct parser_context * ctx);
static struct ast_node * parse_expression(struct parser_context * ctx);
static struct ast_node * parse_if_statement(struct parser_context * ctx);
static struct ast_node * parse_return_statement(struct parser_context * ctx);
static struct ast_node * parse_statement(struct parser_context * ctx);
static struct ast_node * parse_while_statement(struct parser_context * ctx);
static struct ast_node * parse_assignment_expression(struct parser_context * ctx);
static struct ast_node * parse_compound_statement(struct parser_context * ctx);
static struct ast_node * parse_expression_statement(struct parser_context * ctx);
static struct ast_node * parse_function_definition(struct parser_context * ctx);
static struct ast_node * parse_declaration(struct parser_context * ctx);
static struct ast_node * parse_equality_expression(struct parser_context * ctx);
static struct ast_node * parse_relational_expression(struct parser_context * ctx);
static struct ast_node * parse_additive_expression(struct parser_context * ctx);
static struct ast_node * parse_multiplicative_expression(struct parser_context * ctx);
static struct ast_node * parse_cast_expression(struct parser_context * ctx);
static struct ast_node * parse_postfix_expression(struct parser_context * ctx);
static struct ast_node * parse_primary_expression(struct parser_context * ctx);

static void parse_function_parameter_list(struct parser_context * ctx, struct ast_node ** parameters, unsigned int * parameter_count);


struct ast_node * parser_parse(struct parser_context * ctx)
{
    assert(ctx != NULL);

    return parse_translation_unit(ctx);
}

struct ast_node * parse_translation_unit(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct ast_node_list * function_list = NULL;
    struct ast_node_list ** tail = &function_list;

    ctx->current_scope = scope_push(ctx->current_scope, ctx->pool);

    struct token * current_token = parser_get_token(ctx);

    while (current_token != &eos_token) {
        parser_putback_token(current_token, ctx);

        struct ast_node * function_def = parse_function_definition(ctx);

        struct ast_node_list * element = memory_blob_pool_alloc(ctx->pool, sizeof(struct ast_node_list));
        memset(element, 0, sizeof(struct ast_node_list));
        element->node = function_def;
        *tail = element;
        tail = &element->next;

        current_token = parser_get_token(ctx);
    }

    ctx->current_scope = scope_pop(ctx->current_scope);

    struct ast_node * translation_unit = ast_create_node(ctx->pool, AST_NODE_KIND_TRANSLATION_UNIT, &type_void);
    translation_unit->content.translation_unit.list = function_list;
    translation_unit->content.translation_unit.filename = ctx->source_filename;

    return translation_unit;
}

struct ast_node * parse_statement(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * statement = NULL;

    if (token_is_keyword(current_token)) {
        if (strcmp("while", current_token->identifier->name) == 0) {
            statement = parse_while_statement(ctx);
        } else if (strcmp("return", current_token->identifier->name) == 0) {
            statement = parse_return_statement(ctx);
        } else if (strcmp("if", current_token->identifier->name) == 0) {
            statement = parse_if_statement(ctx);
        } else {
            parser_putback_token(current_token, ctx);
            statement = parse_declaration(ctx);
        }
    } else if (token_is_punctuator(current_token, '{')) {
        parser_putback_token(current_token, ctx);
        statement = parse_compound_statement(ctx);
    } else {
        parser_putback_token(current_token, ctx);
        statement = parse_expression_statement(ctx);
    }

    return statement;
}

struct ast_node * parse_compound_statement(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '{')) {
        cclynx_fatal_error("ERROR: expected '{'!\n");
    }

    current_token = parser_get_token(ctx);

    if (token_is_punctuator(current_token, '}')) {
        struct ast_node * compound_statement = ast_create_node(ctx->pool, AST_NODE_KIND_COMPOUND_STATEMENT, &type_void);
        compound_statement->content.list = NULL;
        return compound_statement;
    }

    ctx->current_scope = scope_push(ctx->current_scope, ctx->pool);

    parser_putback_token(current_token, ctx);

    struct ast_node_list * statement_list = NULL;
    struct ast_node_list ** statement_list_end = &statement_list;

    do {
        struct ast_node * statement = parse_statement(ctx);

        struct ast_node_list * list = memory_blob_pool_alloc(ctx->pool, sizeof(struct ast_node_list));
        memset(list, 0, sizeof(struct ast_node_list));
        list->node = statement;
        list->next = NULL;

        *statement_list_end = list;
        statement_list_end = &list->next;

        current_token = parser_get_token(ctx);
        parser_putback_token(current_token, ctx);
    }
    while (!token_is_punctuator(current_token, '}'));

    if (!token_is_punctuator(current_token, '}')) {
        cclynx_fatal_error("ERROR: expected '}'!\n");
    }

    (void)parser_get_token(ctx);

    ctx->current_scope = scope_pop(ctx->current_scope);

    struct ast_node * compound_statement = ast_create_node(ctx->pool, AST_NODE_KIND_COMPOUND_STATEMENT, &type_void);
    compound_statement->content.list = statement_list;

    return compound_statement;
}

struct ast_node * parse_if_statement(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        cclynx_fatal_error("ERROR: expected '('!\n");
    }

    struct ast_node * condition = parse_expression(ctx);

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ')')) {
        cclynx_fatal_error("ERROR: expected ')'!\n");
    }

    struct ast_node * true_branch = parse_statement(ctx);
    struct ast_node * false_branch = NULL;

    current_token = parser_get_token(ctx);

    if (
        token_is_keyword(current_token)
        && strcmp("else", current_token->identifier->name) == 0
    ) {
        false_branch = parse_statement(ctx);
    } else {
        parser_putback_token(current_token, ctx);
    }

    struct ast_node * if_statement = ast_create_node(ctx->pool, AST_NODE_KIND_IF_STATEMENT, &type_void);
    if_statement->content.if_statement.condition = condition;
    if_statement->content.if_statement.true_branch = true_branch;
    if_statement->content.if_statement.false_branch = false_branch;

    return if_statement;
}

struct ast_node * parse_return_statement(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * expression = NULL;

    if (!token_is_punctuator(current_token, ';')) {
        parser_putback_token(current_token, ctx);
        expression = parse_expression(ctx);
        current_token = parser_get_token(ctx);
    }

    if (!token_is_punctuator(current_token, ';')) {
        cclynx_fatal_error("ERROR: expected ';'!\n");
    }

    struct ast_node * return_statement = ast_create_node(ctx->pool, AST_NODE_KIND_RETURN_STATEMENT, &type_void);
    return_statement->content.node = expression;

    return return_statement;
}

struct ast_node * parse_while_statement(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    const struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        cclynx_fatal_error("ERROR: expected '('!\n");
    }

    struct ast_node * expression = parse_expression(ctx);

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ')')) {
        cclynx_fatal_error("ERROR: expected ')'!\n");
    }

    struct ast_node * statement = parse_statement(ctx);

    struct ast_node * while_statement = ast_create_node(ctx->pool, AST_NODE_KIND_WHILE_STATEMENT, &type_void);
    while_statement->content.while_statement.condition = expression;
    while_statement->content.while_statement.body = statement;

    return while_statement;
}

struct ast_node * parse_expression_statement(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * expression = NULL;

    if (!token_is_punctuator(current_token, ';')) {
        parser_putback_token(current_token, ctx);
        expression = parse_expression(ctx);
        current_token = parser_get_token(ctx);
    }

    if (!token_is_punctuator(current_token, ';')) {
        cclynx_fatal_error("ERROR: expected ';'!\n");
    }

    struct ast_node * expression_statement = ast_create_node(ctx->pool, AST_NODE_KIND_EXPRESSION_STATEMENT, &type_void);
    expression_statement->content.node = expression;

    return expression_statement;
}

struct ast_node * parse_function_definition(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        cclynx_fatal_error("ERROR: expected identifier!\n");
    }

    const struct symbol * symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        cclynx_fatal_error("ERROR: expected type specifier!\n");
    }

    current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        cclynx_fatal_error("ERROR: expected identifier!\n");
    }

    if (current_token->identifier->is_keyword) {
        cclynx_fatal_error("ERROR: expected identifier but not a keyword!\n");
    }

    struct identifier * identifier = current_token->identifier;

    struct symbol * function_symbol = scope_find_symbol(ctx->current_scope, identifier, SYMBOL_KIND_FUNCTION);

    if (function_symbol != NULL) {
        cclynx_fatal_error("ERROR: function '%s' already defined!\n", identifier->name);
    }

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        cclynx_fatal_error("ERROR: expected '('!\n");
    }

    function_symbol = memory_blob_pool_alloc(ctx->pool, sizeof(struct symbol));
    memset(function_symbol, 0, sizeof(struct symbol));
    function_symbol->identifier = identifier;
    function_symbol->type = symbol->type;
    function_symbol->kind = SYMBOL_KIND_FUNCTION;

    identifier_attach_symbol(ctx->pool, identifier, function_symbol)
    scope_add_symbol(ctx->current_scope, function_symbol, ctx->pool);

    int parameter_presence = PARAMETER_PRESENCE_UNSPECIFIED;

    current_token = parser_get_token(ctx);

    struct ast_node * parameters[MAX_AST_FUNCTION_PARAMETER_COUNT] = {0};
    unsigned int parameter_count = 0;

    if (
        token_is_keyword(current_token)
        && strcmp("void", current_token->identifier->name) == 0
    ) {
        parameter_presence = PARAMETER_PRESENCE_VOID;
        current_token = parser_get_token(ctx);
    } else if (!token_is_punctuator(current_token, ')')) {
        parser_putback_token(current_token, ctx);
        ctx->current_scope = scope_push(ctx->current_scope, ctx->pool);
        parse_function_parameter_list(ctx, parameters, &parameter_count);
        current_token = parser_get_token(ctx);
        parameter_presence = PARAMETER_PRESENCE_SPECIFIED;
    }

    if (!token_is_punctuator(current_token, ')')) {
        cclynx_fatal_error("ERROR: expected ')'!\n");
    }

    struct ast_node * compound_statement = parse_compound_statement(ctx);

    if (parameter_count > 0) {
        ctx->current_scope = scope_pop(ctx->current_scope);
    }

    struct ast_node * function_definition = ast_create_node(ctx->pool, AST_NODE_KIND_FUNCTION_DEFINITION, symbol->type);
    function_definition->content.function_definition.name = identifier;
    function_definition->content.function_definition.body = compound_statement;
    function_definition->content.function_definition.parameter_presence = parameter_presence;
    memcpy(&function_definition->content.function_definition.parameters, &parameters, sizeof(struct ast_node *) * MAX_AST_FUNCTION_PARAMETER_COUNT);
    function_definition->content.function_definition.parameter_count = parameter_count;

    return function_definition;
}

struct ast_node * parse_declaration(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    const struct token * current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        cclynx_fatal_error("ERROR: expected identifier!\n");
    }

    const struct symbol * symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        cclynx_fatal_error("ERROR: expected type specifier!\n");
    }

    struct type * type = symbol->type;

    current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        cclynx_fatal_error("ERROR: expected identifier!\n");
    }

    if (current_token->identifier->is_keyword) {
        cclynx_fatal_error("ERROR: expected identifier but not a keyword!\n");
    }

    struct identifier * identifier = current_token->identifier;

    symbol = scope_find_symbol(ctx->current_scope, identifier, SYMBOL_KIND_VARIABLE);

    if (symbol != NULL) {
        cclynx_fatal_error("ERROR: variable '%s' already declared!\n", identifier->name);
    }

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ';')) {
        cclynx_fatal_error("ERROR: expected ';'!\n");
    }

    struct symbol * variable = memory_blob_pool_alloc(ctx->pool, sizeof(struct symbol));
    memset(variable, 0, sizeof(struct symbol));
    variable->kind = SYMBOL_KIND_VARIABLE;
    variable->identifier = identifier;
    variable->type = type;

    identifier_attach_symbol(ctx->pool, identifier, variable)
    scope_add_symbol(ctx->current_scope, variable, ctx->pool);

    struct ast_node * declaration = ast_create_node(ctx->pool, AST_NODE_KIND_VARIABLE_DECLARATION, variable->type);
    declaration->content.symbol = variable;

    return declaration;
}

struct ast_node * parse_expression(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    return parse_assignment_expression(ctx);
}

struct ast_node * parse_assignment_expression(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct ast_node * lhs = parse_equality_expression(ctx);

    if (lhs->kind != AST_NODE_KIND_VARIABLE_EXPRESSION) {
        return lhs;
    }

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * initializer = NULL;

    if (token_is_punctuator(current_token, '=')) {
        initializer = parse_equality_expression(ctx);
    } else {
        parser_putback_token(current_token, ctx);
        return lhs;
    }

    struct ast_node * assignment_expression = ast_create_node(ctx->pool, AST_NODE_KIND_ASSIGNMENT_EXPRESSION, &type_void);
    assignment_expression->content.assignment.type = ASSIGNMENT_REGULAR;
    assignment_expression->content.assignment.lhs = lhs;
    assignment_expression->content.assignment.initializer = initializer;

    return assignment_expression;
}

struct ast_node * parse_equality_expression(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct ast_node * lhs = parse_relational_expression(ctx);

    struct token * current_token = parser_get_token(ctx);

    while (
        current_token->kind == TOKEN_KIND_EQUAL_PUNCTUATOR
        || current_token->kind == TOKEN_KIND_NOT_EQUAL_PUNCTUATOR
    ) {
        const enum binary_operation operation = current_token->kind == TOKEN_KIND_EQUAL_PUNCTUATOR
            ? BINARY_OPERATION_EQUALITY
            : BINARY_OPERATION_INEQUALITY;
        struct ast_node * rhs = parse_relational_expression(ctx);

        struct ast_node * binary_expression = ast_create_node(ctx->pool, AST_NODE_KIND_EQUALITY_EXPRESSION, type_resolve(lhs->type, rhs->type));
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(ctx);
    }

    parser_putback_token(current_token, ctx);

    return lhs;
}

struct ast_node * parse_relational_expression(struct parser_context * ctx) {
    assert(ctx!= NULL);

    struct ast_node * lhs = parse_additive_expression(ctx);

    struct token * current_token = parser_get_token(ctx);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            token_first_ch(current_token) == '<'
            || token_first_ch(current_token) == '>'
        )
    ) {
        const enum binary_operation operation = token_first_ch(current_token) == '<'
            ? BINARY_OPERATION_LESS_THAN
            : BINARY_OPERATION_GREATER_THAN;
        struct ast_node * rhs = parse_additive_expression(ctx);

        struct ast_node * binary_expression = ast_create_node(ctx->pool, AST_NODE_KIND_RELATIONAL_EXPRESSION, type_resolve(lhs->type, rhs->type));
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(ctx);
    }

    parser_putback_token(current_token, ctx);

    return lhs;
}


struct ast_node * parse_additive_expression(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct ast_node * lhs = parse_multiplicative_expression(ctx);

    struct token * current_token = parser_get_token(ctx);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            token_first_ch(current_token) == '+'
            || token_first_ch(current_token) == '-'
        )
    ) {
        const enum binary_operation operation = token_first_ch(current_token) == '+'
            ? BINARY_OPERATION_ADDITION
            : BINARY_OPERATION_SUBTRACTION;
        struct ast_node * rhs = parse_multiplicative_expression(ctx);

        struct ast_node * binary_expression = ast_create_node(ctx->pool, AST_NODE_KIND_ADDITIVE_EXPRESSION, type_resolve(lhs->type, rhs->type));
        binary_expression->content.binary_expression.operation = operation;
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
    assert(ctx!= NULL);

    struct ast_node * lhs = parse_cast_expression(ctx);

    struct token * current_token = parser_get_token(ctx);

    while (
        current_token->kind == TOKEN_KIND_PUNCTUATOR
        && (
            token_first_ch(current_token) == '*'
            || token_first_ch(current_token) == '/'
        )
    ) {
        const enum binary_operation operation = token_first_ch(current_token) == '*'
            ? BINARY_OPERATION_MULTIPLY
            : BINARY_OPERATION_DIVIDE;
        struct ast_node * rhs = parse_cast_expression(ctx);

        struct ast_node * binary_expression = ast_create_node(ctx->pool, AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION, type_resolve(lhs->type, rhs->type));
        binary_expression->content.binary_expression.operation = operation;
        binary_expression->content.binary_expression.lhs = lhs;
        binary_expression->content.binary_expression.rhs = rhs;
        lhs = binary_expression;

        current_token = parser_get_token(ctx);
    }

    parser_putback_token(current_token, ctx);

    return lhs;
}

struct ast_node * parse_cast_expression(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        parser_putback_token(current_token, ctx);
        return parse_postfix_expression(ctx);
    }

    struct token * previous_token = current_token;

    current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        parser_putback_token(current_token, ctx);
        parser_putback_token(previous_token, ctx);
        return parse_postfix_expression(ctx);
    }

    const struct symbol * symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        parser_putback_token(current_token, ctx);
        parser_putback_token(previous_token, ctx);
        return parse_postfix_expression(ctx);
    }

    struct type * type = symbol->type;

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ')')) {
        cclynx_fatal_error("ERROR: expected ')'!\n");
    }

    struct ast_node * expression = parse_postfix_expression(ctx);

    struct ast_node * cast_expression = ast_create_node(ctx->pool, AST_NODE_KIND_CAST_EXPRESSION, type);
    cast_expression->content.node = expression;

    return cast_expression;
}

struct ast_node * parse_postfix_expression(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (token_is_identifier(current_token)) {
        struct token * next_token = parser_get_token(ctx);

        if (token_is_punctuator(next_token, '(')) {
            struct symbol * function_symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_FUNCTION);

            if (function_symbol == NULL) {
                cclynx_fatal_error("ERROR: try to call unknown function '%s'\n", current_token->identifier->name);
            }

            next_token = parser_get_token(ctx);

            if (!token_is_punctuator(next_token, ')')) {
                cclynx_fatal_error("ERROR: expected ')'");
            }

            struct ast_node * function_call = ast_create_node(ctx->pool, AST_NODE_KIND_FUNCTION_CALL_EXPRESSION, function_symbol->type);
            function_call->content.function_call.function = function_symbol;
            function_call->content.function_call.argument_count = 0;
            return function_call;
        }

        parser_putback_token(next_token, ctx);
        parser_putback_token(current_token, ctx);

        goto fallback;
    } else {
        parser_putback_token(current_token, ctx);
    }

fallback:
    return parse_primary_expression(ctx);
}

struct ast_node * parse_primary_expression(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    const struct token * current_token = parser_get_token(ctx);

    if (current_token->kind == TOKEN_KIND_NUMBER) {
        const char * number_ptr = current_token->source->content + current_token->span.offset;

        if ((current_token->flags & TOKEN_FLAG_IS_FLOAT) > 0) {
            struct ast_node * number = ast_create_node(ctx->pool, AST_NODE_KIND_FLOAT_CONSTANT_EXPRESSION, &type_float);
            number->content.constant.value.float_constant = (float)strtod(number_ptr, NULL);
            return number;
        }

        struct ast_node * number = ast_create_node(ctx->pool, AST_NODE_KIND_INTEGER_CONSTANT_EXPRESSION, &type_integer);
        number->content.constant.value.integer_constant = (int)strtol(number_ptr, NULL, 10);
        return number;
    }

    if (current_token->kind == TOKEN_KIND_IDENTIFIER) {
        if (current_token->identifier->is_keyword) {
            cclynx_fatal_error("ERROR: expected identifier but not a keyword!\n");
        }

        struct symbol * symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_VARIABLE);

        if (symbol == NULL) {
            cclynx_fatal_error("ERROR: undeclared variable \"%s\"!\n", current_token->identifier->name);
        }

        struct ast_node * variable = ast_create_node(ctx->pool, AST_NODE_KIND_VARIABLE_EXPRESSION, symbol->type);
        variable->content.symbol = symbol;
        return variable;
    }

    if (token_is_punctuator(current_token, '(')) {
        struct ast_node * expression = parse_expression(ctx);

        current_token = parser_get_token(ctx);

        if (!token_is_punctuator(current_token, ')')) {
            cclynx_fatal_error("ERROR: expected ')'!\n");
        }

        return expression;
    }

    cclynx_fatal_error("ERROR: expected number or variable!\n");
}

struct token * parser_get_token(struct parser_context * ctx)
{
    assert(ctx!= NULL);

    if (ctx->token_buffer_pos > 0)
        return ctx->token_buffer[--ctx->token_buffer_pos];

    struct token * current_token = ctx->iterator;
    ctx->iterator = ctx->iterator->next;

    return current_token;
}

void parser_putback_token(struct token * token, struct parser_context * ctx)
{
    assert(token != NULL);
    assert(ctx!= NULL);

    if (token == &eos_token) {
        cclynx_fatal_error("ERROR: Trying to putback EOS token!\n");
    }

    if (ctx->token_buffer_pos >= MAX_TOKEN_BUFFER_SIZE) {
        cclynx_fatal_error("ERROR: Maximum token buffer size reached!\n");
    }

    ctx->token_buffer[ctx->token_buffer_pos++] = token;
}

void parser_init_context(struct parser_context * ctx, struct token * tokens, struct memory_blob_pool * pool, struct scope * file_scope, const char * source_filename)
{
    assert(ctx!= NULL);
    assert(tokens != NULL);
    assert(pool != NULL);

    memset(ctx, 0, sizeof(struct parser_context));
    ctx->pool = pool;
    ctx->tokens = tokens;
    ctx->iterator = ctx->tokens;
    ctx->current_scope = file_scope;
    ctx->source_filename = source_filename;
}


struct ast_node * parse_function_parameter(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        return NULL;
    }

    const struct symbol * symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_TYPE_SPECIFIER);

    if (symbol == NULL) {
        cclynx_fatal_error("ERROR: expected type specifier!\n");
    }

    struct type * parameter_type = symbol->type;

    current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        cclynx_fatal_error("ERROR: expected identifier!\n");
    }

    if (current_token->identifier->is_keyword) {
        cclynx_fatal_error("ERROR: expected identifier but not a keyword!\n");
    }

    struct identifier * parameter_identifier = current_token->identifier;

    symbol = scope_find_symbol(ctx->current_scope, parameter_identifier, SYMBOL_KIND_VARIABLE);

    if (symbol != NULL) {
        cclynx_fatal_error("ERROR: parameter '%s' already declared!\n", parameter_identifier->name);
    }

    struct symbol * parameter_symbol = memory_blob_pool_alloc(ctx->pool, sizeof(struct symbol));
    memset(parameter_symbol, 0, sizeof(struct symbol));
    parameter_symbol->kind = SYMBOL_KIND_VARIABLE;
    parameter_symbol->flags = SYMBOL_FLAG_FUNCTION_PARAMETER;
    parameter_symbol->type = parameter_type;
    parameter_symbol->identifier = parameter_identifier;

    identifier_attach_symbol(ctx->pool, parameter_identifier, parameter_symbol)
    scope_add_symbol(ctx->current_scope, parameter_symbol, ctx->pool);

    struct ast_node * parameter = ast_create_node(ctx->pool, AST_NODE_KIND_FUNCTION_PARAMETER, parameter_symbol->type);
    parameter->content.symbol = parameter_symbol;

    return parameter;
}

void parse_function_parameter_list(struct parser_context * ctx, struct ast_node ** parameters, unsigned int * parameter_count)
{
    assert(ctx != NULL);
    assert(parameters != NULL);
    assert(parameter_count != NULL);

    struct token * token;

    do {
        struct ast_node * parameter = parse_function_parameter(ctx);

        if (parameter == NULL) {
            return;
        }

        parameters[(*parameter_count)++] = parameter;
        token = parser_get_token(ctx);

        if (token_is_punctuator(token, ',') && *parameter_count >= MAX_AST_FUNCTION_PARAMETER_COUNT) {
            cclynx_fatal_error("ERROR: only 3 parameters per function are supported for now!\n");
        }
    } while (token_is_punctuator(token, ','));

    parser_putback_token(token, ctx);
}
