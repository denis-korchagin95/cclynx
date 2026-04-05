#include <assert.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "warning.h"
#include "binary_expression_parser.h"
#include "tokenizer.h"
#include "allocator.h"
#include "identifier.h"
#include "scope.h"
#include "symbol.h"
#include "type.h"
#include "error.h"


struct declaration_specifiers
{
    enum type_kind type_kind;
    unsigned int modifiers;
};


static struct ast_node * parse_translation_unit(struct parser_context * ctx);
static struct ast_node * parse_expression(struct parser_context * ctx);
static struct ast_node * parse_if_statement(struct parser_context * ctx, struct token * keyword_token);
static struct ast_node * parse_return_statement(struct parser_context * ctx);
static struct ast_node * parse_statement(struct parser_context * ctx);
static struct ast_node * parse_while_statement(struct parser_context * ctx, struct token * keyword_token);
static struct ast_node * parse_assignment_expression(struct parser_context * ctx);
static struct ast_node * parse_compound_statement(struct parser_context * ctx);
static struct ast_node * parse_expression_statement(struct parser_context * ctx);
static struct ast_node * parse_function_definition(struct parser_context * ctx);
static struct ast_node * parse_declaration(struct parser_context * ctx);
static struct ast_node * parse_postfix_expression(struct parser_context * ctx);
static struct ast_node * parse_primary_expression(struct parser_context * ctx);

static void parse_function_parameter_list(struct parser_context * ctx, struct ast_node ** parameters, unsigned int * parameter_count);
static struct ast_node * parse_number(struct parser_context * ctx, const struct token * token);
void parse_declaration_specifiers(struct parser_context * ctx, struct declaration_specifiers * specifiers);
struct type * resolve_type(struct declaration_specifiers * specifiers);


static void parser_report_error(struct parser_context * ctx, const struct token * token, const char * fmt, ...)
{
    assert(ctx != NULL);
    assert(token != NULL);
    assert(fmt != NULL);

    ctx->has_error = true;

    char message[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    error_list_add(&ctx->errors, "%s:%u:%u: ERROR: %s\n",
        ctx->source_filename,
        token->span.position.line,
        token->span.position.column,
        message);
}

static void parser_report_warning_v(struct parser_context * ctx, enum warning_code code, struct source_position position, const char * fmt, va_list args)
{
    assert(ctx != NULL);
    assert(fmt != NULL);

    if (!warning_is_enabled(&ctx->warning_flags, code)) {
        return;
    }

    char message[512];
    vsnprintf(message, sizeof(message), fmt, args);

    error_list_add(&ctx->errors, "%s:%u:%u: WARNING: %s\n",
        ctx->source_filename,
        position.line,
        position.column,
        message);
}

void parser_report_warning(struct parser_context * ctx, enum warning_code code, const struct token * token, const char * fmt, ...)
{
    assert(ctx != NULL);
    assert(token != NULL);
    assert(fmt != NULL);

    va_list args;
    va_start(args, fmt);
    parser_report_warning_v(ctx, code, token->span.position, fmt, args);
    va_end(args);
}

static void parser_report_warning_at(struct parser_context * ctx, enum warning_code code, struct source_position position, const char * fmt, ...)
{
    assert(ctx != NULL);
    assert(fmt != NULL);

    va_list args;
    va_start(args, fmt);
    parser_report_warning_v(ctx, code, position, fmt, args);
    va_end(args);
}

static void parser_synchronize_statement(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * token = parser_get_token(ctx);
    while (token != &eos_token) {
        if (token_is_punctuator(token, ';')) {
            return;
        }
        if (token_is_punctuator(token, '}')) {
            parser_putback_token(token, ctx);
            return;
        }
        token = parser_get_token(ctx);
    }
}

static void parser_synchronize_toplevel(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * token = parser_get_token(ctx);
    while (token != &eos_token) {
        if (token_is_punctuator(token, '}')) {
            return;
        }
        token = parser_get_token(ctx);
    }
    parser_putback_token(token, ctx);
}

static void report_unused_variables(struct parser_context * ctx)
{
    assert(ctx != NULL);

    const struct symbol_list * it = ctx->current_scope->symbols;
    while (it != NULL) {
        if (it->symbol->kind == SYMBOL_KIND_VARIABLE && !(it->symbol->flags & SYMBOL_FLAG_USED)) {
            bool is_parameter = (it->symbol->flags & SYMBOL_FLAG_FUNCTION_PARAMETER) != 0;
            enum warning_code code = is_parameter ? WARNING_UNUSED_PARAMETER : WARNING_UNUSED_VARIABLE;
            const char * kind = is_parameter ? "parameter" : "variable";
            parser_report_warning_at(ctx, code, it->symbol->declaration_position, "unused %s '%s'", kind, it->symbol->identifier->name);
        }
        it = it->next;
    }
}


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

        if (function_def != NULL) {
            struct ast_node_list * element = memory_blob_pool_alloc(ctx->pool, sizeof(struct ast_node_list));
            element->node = function_def;
            *tail = element;
            tail = &element->next;
        }

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
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * statement = NULL;

    if (token_is_keyword(current_token)) {
        if (current_token->identifier->keyword_code == KEYWORD_WHILE) {
            statement = parse_while_statement(ctx, current_token);
        } else if (current_token->identifier->keyword_code == KEYWORD_RETURN) {
            statement = parse_return_statement(ctx);
        } else if (current_token->identifier->keyword_code == KEYWORD_IF) {
            statement = parse_if_statement(ctx, current_token);
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
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '{')) {
        parser_report_error(ctx, current_token, "expected '{' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        return NULL;
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

    for (;;) {
        current_token = parser_get_token(ctx);
        if (token_is_punctuator(current_token, '}') || current_token == &eos_token) {
            break;
        }
        struct token * stmt_token = current_token;
        parser_putback_token(current_token, ctx);

        struct ast_node * statement = parse_statement(ctx);

        if (ast_is_empty_compound_statement(statement)) {
            parser_report_warning(ctx, WARNING_EMPTY_COMPOUND_STATEMENT, stmt_token, "empty compound statement");
        }

        if (statement != NULL) {
            struct ast_node_list * list = memory_blob_pool_alloc(ctx->pool, sizeof(struct ast_node_list));
            list->node = statement;
            list->next = NULL;

            *statement_list_end = list;
            statement_list_end = &list->next;
        }
    }

    if (current_token == &eos_token) {
        parser_report_error(ctx, current_token, "expected '}' but got %s", token_stringify(current_token));
    }

    report_unused_variables(ctx);
    ctx->current_scope = scope_pop(ctx->current_scope);

    struct ast_node * compound_statement = ast_create_node(ctx->pool, AST_NODE_KIND_COMPOUND_STATEMENT, &type_void);
    compound_statement->content.list = statement_list;

    return compound_statement;
}

struct ast_node * parse_if_statement(struct parser_context * ctx, struct token * keyword_token)
{
    assert(ctx != NULL);
    assert(keyword_token != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        parser_report_error(ctx, current_token, "expected '(' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct ast_node * condition = parse_expression(ctx);

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ')')) {
        parser_report_error(ctx, current_token, "expected ')' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct ast_node * true_branch = parse_statement(ctx);

    if (ast_is_empty_compound_statement(true_branch)) {
        parser_report_warning(ctx, WARNING_EMPTY_IF_BODY, keyword_token, "empty if body");
    }

    struct ast_node * false_branch = NULL;

    current_token = parser_get_token(ctx);

    if (
        token_is_keyword(current_token)
        && strcmp("else", current_token->identifier->name) == 0
    ) {
        struct token * else_token = current_token;
        false_branch = parse_statement(ctx);

        if (ast_is_empty_compound_statement(false_branch)) {
            parser_report_warning(ctx, WARNING_EMPTY_ELSE_BODY, else_token, "empty else body");
        }
    } else {
        parser_putback_token(current_token, ctx);
    }

    if (condition == NULL || true_branch == NULL) {
        return NULL;
    }

    struct ast_node * if_statement = ast_create_node(ctx->pool, AST_NODE_KIND_IF_STATEMENT, &type_void);
    if_statement->content.if_statement.condition = condition;
    if_statement->content.if_statement.true_branch = true_branch;
    if_statement->content.if_statement.false_branch = false_branch;

    return if_statement;
}

struct ast_node * parse_return_statement(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * expression = NULL;

    if (!token_is_punctuator(current_token, ';')) {
        parser_putback_token(current_token, ctx);
        expression = parse_expression(ctx);

        if (expression == NULL) {
            parser_synchronize_statement(ctx);
            return NULL;
        }

        current_token = parser_get_token(ctx);
    }

    if (!token_is_punctuator(current_token, ';')) {
        parser_report_error(ctx, current_token, "expected ';' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    if (
        expression != NULL
        && ctx->current_function != NULL
        && expression->type->kind == ctx->current_function->type->kind
        && type_signedness_differs(expression->type, ctx->current_function->type)
    ) {
        parser_report_warning(ctx, WARNING_SIGN_CONVERSION, current_token,
            "implicit conversion changes signedness from '%s' to '%s', use an explicit cast",
            type_stringify(expression->type), type_stringify(ctx->current_function->type));
        struct ast_node * cast = ast_create_node(ctx->pool, AST_NODE_KIND_CAST_EXPRESSION, ctx->current_function->type);
        cast->content.node = expression;
        expression = cast;
    }

    struct ast_node * return_statement = ast_create_node(ctx->pool, AST_NODE_KIND_RETURN_STATEMENT, &type_void);
    return_statement->content.node = expression;

    return return_statement;
}

struct ast_node * parse_while_statement(struct parser_context * ctx, struct token * keyword_token)
{
    assert(ctx != NULL);
    assert(keyword_token != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        parser_report_error(ctx, current_token, "expected '(' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct ast_node * expression = parse_expression(ctx);

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ')')) {
        parser_report_error(ctx, current_token, "expected ')' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct ast_node * statement = parse_statement(ctx);

    if (ast_is_empty_compound_statement(statement)) {
        parser_report_warning(ctx, WARNING_EMPTY_WHILE_BODY, keyword_token, "empty while body");
    }

    if (expression == NULL || statement == NULL) {
        return NULL;
    }

    struct ast_node * while_statement = ast_create_node(ctx->pool, AST_NODE_KIND_WHILE_STATEMENT, &type_void);
    while_statement->content.while_statement.condition = expression;
    while_statement->content.while_statement.body = statement;

    return while_statement;
}

struct ast_node * parse_expression_statement(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * expression = NULL;

    if (!token_is_punctuator(current_token, ';')) {
        parser_putback_token(current_token, ctx);
        expression = parse_expression(ctx);

        if (expression == NULL) {
            parser_synchronize_statement(ctx);
            return NULL;
        }

        current_token = parser_get_token(ctx);
    }

    if (!token_is_punctuator(current_token, ';')) {
        parser_report_error(ctx, current_token, "expected ';' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct ast_node * expression_statement = ast_create_node(ctx->pool, AST_NODE_KIND_EXPRESSION_STATEMENT, &type_void);
    expression_statement->content.node = expression;

    return expression_statement;
}

struct ast_node * parse_function_definition(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct declaration_specifiers specifiers;
    memset(&specifiers, 0, sizeof(struct declaration_specifiers));

    parse_declaration_specifiers(ctx, &specifiers);

    struct type * type = resolve_type(&specifiers);

    if (type == NULL) {
        struct token * error_token = parser_get_token(ctx);
        parser_report_error(ctx, error_token, "expected type specifier but got %s", token_stringify(error_token));
        if (error_token != &eos_token) {
            parser_putback_token(error_token, ctx);
        }
        parser_synchronize_toplevel(ctx);
        return NULL;
    }

    struct token * current_token = parser_get_token(ctx);
    struct token * name_token = current_token;

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        parser_report_error(ctx, current_token, "expected identifier but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_toplevel(ctx);
        return NULL;
    }

    if (token_is_keyword(current_token)) {
        parser_report_error(ctx, current_token, "expected identifier but got keyword '%s'", current_token->identifier->name);
        parser_synchronize_toplevel(ctx);
        return NULL;
    }

    struct identifier * identifier = current_token->identifier;

    struct symbol * function_symbol = scope_find_symbol(ctx->current_scope, identifier, SYMBOL_KIND_FUNCTION);

    if (function_symbol != NULL) {
        parser_report_error(ctx, current_token, "function '%s' already defined", identifier->name);
    }

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        parser_report_error(ctx, current_token, "expected '(' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_toplevel(ctx);
        return NULL;
    }

    function_symbol = memory_blob_pool_alloc(ctx->pool, sizeof(struct symbol));
    function_symbol->identifier = identifier;
    function_symbol->type = type;
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
        parser_report_error(ctx, current_token, "expected ')' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_toplevel(ctx);
        if (parameter_presence == PARAMETER_PRESENCE_SPECIFIED) {
            report_unused_variables(ctx);
            ctx->current_scope = scope_pop(ctx->current_scope);
        }
        return NULL;
    }

    function_symbol->parameter_presence = parameter_presence;
    function_symbol->parameter_count = parameter_count;
    for (unsigned int i = 0; i < parameter_count; i++) {
        function_symbol->parameters[i] = parameters[i]->content.symbol;
        parameters[i]->content.symbol->parameter_index = i;
    }

    struct symbol * saved_function = ctx->current_function;
    ctx->current_function = function_symbol;

    struct ast_node * compound_statement = parse_compound_statement(ctx);

    ctx->current_function = saved_function;

    if (parameter_count > 0) {
        report_unused_variables(ctx);
        ctx->current_scope = scope_pop(ctx->current_scope);
    }

    if (compound_statement != NULL && ast_is_empty_compound_statement(compound_statement)) {
        parser_report_warning(ctx, WARNING_EMPTY_FUNCTION_BODY, name_token, "function '%s' has empty body", identifier->name);
    } else if (type != &type_void && compound_statement != NULL && !ast_statement_always_returns(compound_statement)) {
        parser_report_warning(ctx, WARNING_MISSING_RETURN, name_token, "function '%s' missing return statement", identifier->name);
    }

    struct ast_node * function_definition = ast_create_node(ctx->pool, AST_NODE_KIND_FUNCTION_DEFINITION, type);
    function_definition->content.function_definition.name = identifier;
    function_definition->content.function_definition.body = compound_statement;
    function_definition->content.function_definition.parameter_presence = parameter_presence;
    memcpy(&function_definition->content.function_definition.parameters, &parameters, sizeof(struct ast_node *) * MAX_AST_FUNCTION_PARAMETER_COUNT);
    function_definition->content.function_definition.parameter_count = parameter_count;

    return function_definition;
}

struct ast_node * parse_declaration(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct declaration_specifiers specifiers;
    memset(&specifiers, 0, sizeof(struct declaration_specifiers));

    parse_declaration_specifiers(ctx, &specifiers);

    struct type * type = resolve_type(&specifiers);

    if (type == NULL) {
        struct token * error_token = parser_get_token(ctx);
        parser_report_error(ctx, error_token, "expected type specifier but got %s", token_stringify(error_token));
        if (error_token != &eos_token) {
            parser_putback_token(error_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    if (type->kind == TYPE_KIND_VOID) {
        struct token * name_token = parser_get_token(ctx);
        parser_report_error(ctx, name_token, "variable cannot have 'void' type");
        if (name_token != &eos_token) {
            parser_putback_token(name_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct token * current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        parser_report_error(ctx, current_token, "expected identifier but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    if (token_is_keyword(current_token)) {
        parser_report_error(ctx, current_token, "expected identifier but got keyword '%s'", current_token->identifier->name);
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct identifier * identifier = current_token->identifier;

    const struct symbol * symbol = scope_find_symbol(ctx->current_scope, identifier, SYMBOL_KIND_VARIABLE);

    if (symbol != NULL) {
        parser_report_error(ctx, current_token, "variable '%s' already declared", identifier->name);
    }

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ';')) {
        parser_report_error(ctx, current_token, "expected ';' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        parser_synchronize_statement(ctx);
        return NULL;
    }

    struct symbol * variable = memory_blob_pool_alloc(ctx->pool, sizeof(struct symbol));
    variable->kind = SYMBOL_KIND_VARIABLE;
    variable->identifier = identifier;
    variable->type = type;
    variable->declaration_position.line = current_token->span.position.line;
    variable->declaration_position.column = current_token->span.position.column;

    identifier_attach_symbol(ctx->pool, identifier, variable)
    scope_add_symbol(ctx->current_scope, variable, ctx->pool);

    struct ast_node * declaration = ast_create_node(ctx->pool, AST_NODE_KIND_VARIABLE_DECLARATION, variable->type);
    declaration->content.symbol = variable;

    return declaration;
}

struct ast_node * parse_expression(struct parser_context * ctx)
{
    assert(ctx != NULL);

    return parse_assignment_expression(ctx);
}

struct ast_node * parse_assignment_expression(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct ast_node * lhs = parse_equality_expression(ctx);

    if (lhs == NULL || lhs->kind != AST_NODE_KIND_VARIABLE_EXPRESSION) {
        return lhs;
    }

    struct token * current_token = parser_get_token(ctx);

    struct ast_node * initializer = NULL;

    if (token_is_punctuator(current_token, '=')) {
        initializer = parse_equality_expression(ctx);
        if (initializer == NULL) {
            return NULL;
        }
    } else {
        parser_putback_token(current_token, ctx);
        return lhs;
    }

    if (
        lhs->type->kind == initializer->type->kind
        && type_signedness_differs(lhs->type, initializer->type)
    ) {
        parser_report_warning(ctx, WARNING_SIGN_CONVERSION, current_token,
            "implicit conversion changes signedness from '%s' to '%s', use an explicit cast",
            type_stringify(initializer->type), type_stringify(lhs->type));
        struct ast_node * cast = ast_create_node(ctx->pool, AST_NODE_KIND_CAST_EXPRESSION, lhs->type);
        cast->content.node = initializer;
        initializer = cast;
    }

    struct ast_node * assignment_expression = ast_create_node(ctx->pool, AST_NODE_KIND_ASSIGNMENT_EXPRESSION, &type_void);
    assignment_expression->content.assignment.type = ASSIGNMENT_REGULAR;
    assignment_expression->content.assignment.lhs = lhs;
    assignment_expression->content.assignment.initializer = initializer;

    return assignment_expression;
}

struct ast_node * parse_cast_expression(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, '(')) {
        parser_putback_token(current_token, ctx);
        return parse_postfix_expression(ctx);
    }

    struct token * previous_token = current_token;

    struct declaration_specifiers specifiers;
    memset(&specifiers, 0, sizeof(struct declaration_specifiers));

    parse_declaration_specifiers(ctx, &specifiers);

    struct type * type = resolve_type(&specifiers);

    if (type == NULL) {
        parser_putback_token(previous_token, ctx);
        return parse_postfix_expression(ctx);
    }

    current_token = parser_get_token(ctx);

    if (!token_is_punctuator(current_token, ')')) {
        parser_report_error(ctx, current_token, "expected ')' but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        return NULL;
    }

    struct ast_node * expression = parse_postfix_expression(ctx);

    if (expression == NULL) {
        return NULL;
    }

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
                parser_report_error(ctx, current_token, "call to undeclared function '%s'", current_token->identifier->name);

                next_token = parser_get_token(ctx);
                while (!token_is_punctuator(next_token, ')') && next_token != &eos_token) {
                    next_token = parser_get_token(ctx);
                }
                return NULL;
            }

            struct ast_node * arguments[MAX_AST_FUNCTION_ARGUMENT_COUNT] = {0};
            unsigned int argument_count = 0;

            next_token = parser_get_token(ctx);

            if (!token_is_punctuator(next_token, ')')) {
                parser_putback_token(next_token, ctx);

                do {
                    struct ast_node * argument = parse_assignment_expression(ctx);
                    arguments[argument_count++] = argument;
                    next_token = parser_get_token(ctx);

                    if (token_is_punctuator(next_token, ',') && argument_count >= MAX_AST_FUNCTION_ARGUMENT_COUNT) {
                        parser_report_error(ctx, next_token, "too many arguments, maximum is 3");
                        while (!token_is_punctuator(next_token, ')') && next_token != &eos_token) {
                            next_token = parser_get_token(ctx);
                        }
                        return NULL;
                    }
                } while (token_is_punctuator(next_token, ','));

                if (!token_is_punctuator(next_token, ')')) {
                    parser_report_error(ctx, next_token, "expected ')' but got %s", token_stringify(next_token));
                    if (next_token != &eos_token) {
                        parser_putback_token(next_token, ctx);
                    }
                    return NULL;
                }
            }

            if (function_symbol->parameter_presence == PARAMETER_PRESENCE_UNSPECIFIED && argument_count > 0) {
                parser_report_warning(ctx, WARNING_UNSPECIFIED_PARAMETERS, current_token,
                    "function '%s' has unspecified parameters, consider using '%s(void)' or adding parameter types",
                    function_symbol->identifier->name, function_symbol->identifier->name);
            }

            if (function_symbol->parameter_presence != PARAMETER_PRESENCE_UNSPECIFIED) {
                if (function_symbol->parameter_count != argument_count) {
                    parser_report_error(ctx, current_token, "function '%s' expects %u arguments but %u were provided",
                        function_symbol->identifier->name, function_symbol->parameter_count, argument_count);
                    return NULL;
                }

                for (unsigned int i = 0; i < argument_count; i++) {
                    if (arguments[i] == NULL) {
                        continue;
                    }
                    struct type * arg_type = arguments[i]->type;
                    struct type * param_type = function_symbol->parameters[i]->type;
                    if (arg_type == param_type) {
                        continue;
                    }
                    if (
                        arg_type->kind == param_type->kind
                        && type_signedness_differs(arg_type, param_type)
                    ) {
                        parser_report_warning(ctx, WARNING_SIGN_CONVERSION, current_token,
                            "implicit conversion changes signedness from '%s' to '%s', use an explicit cast",
                            type_stringify(arg_type), type_stringify(param_type));
                        struct ast_node * cast = ast_create_node(ctx->pool, AST_NODE_KIND_CAST_EXPRESSION, param_type);
                        cast->content.node = arguments[i];
                        arguments[i] = cast;
                    } else {
                        parser_report_error(ctx, current_token, "argument %u of function '%s' has type '%s' but expected '%s'",
                            i + 1, function_symbol->identifier->name,
                            type_stringify(arg_type),
                            type_stringify(param_type));
                    }
                }
            }

            struct ast_node * function_call = ast_create_node(ctx->pool, AST_NODE_KIND_FUNCTION_CALL_EXPRESSION, function_symbol->type);
            function_call->content.function_call.function = function_symbol;
            function_call->content.function_call.argument_count = argument_count;
            memcpy(&function_call->content.function_call.arguments, &arguments, sizeof(struct ast_node *) * MAX_AST_FUNCTION_ARGUMENT_COUNT);
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

struct ast_node * parse_number(struct parser_context * ctx, const struct token * token)
{
    assert(ctx != NULL);
    assert(token != NULL);
    assert(token->kind == TOKEN_KIND_NUMBER);

    const char * ptr = token->source->content + token->span.offset;
    int is_unsigned = token->flags & TOKEN_FLAG_IS_UNSIGNED;

    long long int value = 0;
    for (uint32_t i = 0; i < token->span.length; ++i) {
        char ch = ptr[i];
        if (ch >= '0' && ch <= '9') {
            value = value * 10 + (ch - '0');
        }
    }

    struct type * type = is_unsigned ? &type_uint32 : &type_sint32;
    struct ast_node * number = ast_create_node(ctx->pool, AST_NODE_KIND_INTEGER_CONSTANT_EXPRESSION, type);
    number->content.constant.value = value;

    return number;
}

struct ast_node * parse_primary_expression(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct token * current_token = parser_get_token(ctx);

    if (current_token->kind == TOKEN_KIND_NUMBER) {
        return parse_number(ctx, current_token);
    }

    if (current_token->kind == TOKEN_KIND_IDENTIFIER) {
        if (token_is_keyword(current_token)) {
            parser_report_error(ctx, current_token, "expected identifier but got keyword '%s'", current_token->identifier->name);
            return NULL;
        }

        struct symbol * symbol = symbol_lookup(current_token->identifier, SYMBOL_KIND_VARIABLE);

        if (symbol == NULL) {
            parser_report_error(ctx, current_token, "undeclared variable '%s'", current_token->identifier->name);
            return NULL;
        }

        symbol->flags |= SYMBOL_FLAG_USED;

        struct ast_node * variable = ast_create_node(ctx->pool, AST_NODE_KIND_VARIABLE_EXPRESSION, symbol->type);
        variable->content.symbol = symbol;
        return variable;
    }

    if (token_is_punctuator(current_token, '(')) {
        struct ast_node * expression = parse_expression(ctx);

        current_token = parser_get_token(ctx);

        if (!token_is_punctuator(current_token, ')')) {
            parser_report_error(ctx, current_token, "expected ')' but got %s", token_stringify(current_token));
            if (current_token != &eos_token) {
                parser_putback_token(current_token, ctx);
            }
            return NULL;
        }

        return expression;
    }

    parser_report_error(ctx, current_token, "expected expression but got %s", token_stringify(current_token));

    if (current_token != &eos_token) {
        parser_putback_token(current_token, ctx);
    }

    return NULL;
}

struct token * parser_get_token(struct parser_context * ctx)
{
    assert(ctx != NULL);

    if (ctx->token_buffer_pos > 0)
        return ctx->token_buffer[--ctx->token_buffer_pos];

    struct token * current_token = ctx->iterator;
    ctx->iterator = ctx->iterator->next;

    return current_token;
}

void parser_putback_token(struct token * token, struct parser_context * ctx)
{
    assert(token != NULL);
    assert(ctx != NULL);

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
    assert(ctx != NULL);
    assert(tokens != NULL);
    assert(pool != NULL);

    memset(ctx, 0, sizeof(struct parser_context));
    ctx->pool = pool;
    ctx->tokens = tokens;
    ctx->iterator = ctx->tokens;
    ctx->current_scope = file_scope;
    ctx->source_filename = source_filename;
    error_list_init(&ctx->errors, pool);
    ctx->has_error = false;
}

struct ast_node * parse_function_parameter(struct parser_context * ctx)
{
    assert(ctx != NULL);

    struct declaration_specifiers specifiers;
    memset(&specifiers, 0, sizeof(struct declaration_specifiers));

    parse_declaration_specifiers(ctx, &specifiers);

    struct type * parameter_type = resolve_type(&specifiers);

    if (parameter_type == NULL) {
        struct token * next = parser_get_token(ctx);
        parser_putback_token(next, ctx);
        if (token_is_identifier(next)) {
            parser_report_error(ctx, next, "expected type specifier but got %s", token_stringify(next));
        }
        return NULL;
    }

    struct token * current_token = parser_get_token(ctx);

    if (current_token->kind != TOKEN_KIND_IDENTIFIER) {
        parser_report_error(ctx, current_token, "expected identifier but got %s", token_stringify(current_token));
        if (current_token != &eos_token) {
            parser_putback_token(current_token, ctx);
        }
        return NULL;
    }

    if (token_is_keyword(current_token)) {
        parser_report_error(ctx, current_token, "expected identifier but got keyword '%s'", current_token->identifier->name);
        return NULL;
    }

    struct identifier * parameter_identifier = current_token->identifier;

    const struct symbol * existing_symbol = scope_find_symbol(ctx->current_scope, parameter_identifier, SYMBOL_KIND_VARIABLE);

    if (existing_symbol != NULL) {
        parser_report_error(ctx, current_token, "parameter '%s' already declared", parameter_identifier->name);
    }

    struct symbol * parameter_symbol = memory_blob_pool_alloc(ctx->pool, sizeof(struct symbol));
    parameter_symbol->kind = SYMBOL_KIND_VARIABLE;
    parameter_symbol->flags = SYMBOL_FLAG_FUNCTION_PARAMETER;
    parameter_symbol->type = parameter_type;
    parameter_symbol->identifier = parameter_identifier;
    parameter_symbol->declaration_position.line = current_token->span.position.line;
    parameter_symbol->declaration_position.column = current_token->span.position.column;

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
            parser_report_error(ctx, token, "too many parameters, maximum is 3");
            while (!token_is_punctuator(token, ')') && token != &eos_token) {
                token = parser_get_token(ctx);
            }
            parser_putback_token(token, ctx);
            return;
        }
    } while (token_is_punctuator(token, ','));

    parser_putback_token(token, ctx);
}

void parse_declaration_specifiers(struct parser_context * ctx, struct declaration_specifiers * specifiers)
{
    assert(ctx != NULL);
    assert(specifiers != NULL);

    struct token * current_token = parser_get_token(ctx);

    while (current_token->kind == TOKEN_KIND_IDENTIFIER) {
        if (current_token->identifier->keyword_code == KEYWORD_NONE) {
            break;
        }

        switch (current_token->identifier->keyword_code) {
            case KEYWORD_VOID:
            case KEYWORD_INT:
                if (specifiers->type_kind != TYPE_KIND_UNDEFINED) {
                    parser_report_error(ctx, current_token, "mixed types");
                    current_token = parser_get_token(ctx);
                    continue;
                }
                if ((specifiers->modifiers & TYPE_MODIFIER_UNSIGNED) && current_token->identifier->keyword_code == KEYWORD_VOID) {
                    parser_report_error(ctx, current_token, "'unsigned' cannot be applied to 'void'");
                    current_token = parser_get_token(ctx);
                    continue;
                }
                break;
            case KEYWORD_UNSIGNED:
                if (specifiers->modifiers & TYPE_MODIFIER_UNSIGNED) {
                    parser_report_error(ctx, current_token, "duplicate 'unsigned' keyword");
                    current_token = parser_get_token(ctx);
                    continue;
                }
                if (specifiers->type_kind == TYPE_KIND_VOID) {
                    parser_report_error(ctx, current_token, "'unsigned' cannot be applied to 'void'");
                    current_token = parser_get_token(ctx);
                    continue;
                }
                break;
            default:
                goto done;
        }

        switch (current_token->identifier->keyword_code) {
            case KEYWORD_VOID:
                specifiers->type_kind = TYPE_KIND_VOID;
                break;
            case KEYWORD_INT:
                specifiers->type_kind = TYPE_KIND_INTEGER;
                break;
            case KEYWORD_UNSIGNED:
                specifiers->modifiers |= TYPE_MODIFIER_UNSIGNED;
                break;
            default:
                goto done;
        }

        current_token = parser_get_token(ctx);
    }

done:
    parser_putback_token(current_token, ctx);
}

struct type * resolve_type(struct declaration_specifiers * specifiers)
{
    assert(specifiers != NULL);

    if (specifiers->type_kind == TYPE_KIND_UNDEFINED) {
        if (specifiers->modifiers != 0) {
            specifiers->type_kind = TYPE_KIND_INTEGER;
        } else {
            return NULL;
        }
    }

    if (specifiers->type_kind == TYPE_KIND_VOID) {
        return &type_void;
    }

    if (specifiers->type_kind == TYPE_KIND_INTEGER) {
        if (specifiers->modifiers & TYPE_MODIFIER_UNSIGNED) {
            return &type_uint32;
        }
        return &type_sint32;
    }

    cclynx_fatal_error("FATAL ERROR: unhandled type\n");

    return NULL;
}
