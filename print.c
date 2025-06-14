#include <assert.h>

#include "print.h"

#include "tokenizer.h"
#include "identifier.h"
#include "parser.h"

static void do_print_ast(const struct ast_node * ast, FILE * file, int depth, unsigned int * ancestors_info);

void print_token(const struct token * token, FILE * file)
{
    assert(token != NULL);
    assert(file != NULL);

    switch (token->kind) {
        case TOKEN_KIND_PUNCTUATOR:
            fprintf(file, "<TOKEN_PUNCTUATOR '%c'>\n", token->content.ch);
            break;
        case TOKEN_KIND_EOS:
            fprintf(file, "<TOKEN_EOS>\n");
            break;
        case TOKEN_KIND_UNKNOWN_CHARACTER:
            fprintf(file, "<TOKEN_UNKNOWN_CHARACTER '%c'>\n", token->content.ch);
            break;
        case TOKEN_KIND_IDENTIFIER:
            fprintf(file, "<TOKEN_%s '%s'>\n", token->content.identifier->is_keyword ? "KEYWORD" : "IDENTIFIER", token->content.identifier->name);
            break;
        case TOKEN_KIND_NUMBER:
            fprintf(file, "<TOKEN_NUMBER '%lld'>\n", token->content.integer_constant);
            break;
        case TOKEN_KIND_EQUAL_PUNCTUATOR:
            fprintf(file, "<TOKEN_SPECIAL_PUNCTUATOR '=='>\n");
            break;
        case TOKEN_KIND_NOT_EQUAL_PUNCTUATOR:
            fprintf(file, "<TOKEN_SPECIAL_PUNCTUATOR '!='>\n");
            break;
        default:
            fprintf(file, "<TOKEN_UNKNOWN>\n");
    }
}


void print_ast(const struct ast_node * ast, FILE * file)
{
    assert(ast != NULL);
    assert(file != NULL);

    unsigned int ancestors_info[512] = {0};

    ancestors_info[0] = 1;

	do_print_ast(ast, file, 0, ancestors_info);
}

void do_print_ast(const struct ast_node * ast, FILE * file, int depth, unsigned int * ancestors_info)
{
    assert(ast != NULL);
    assert(file != NULL);

    const unsigned int is_top = (ancestors_info[depth] & 1) > 0;

    if (is_top == 0) {
        for (int i = 0; i < depth - 1; ++i) {
            const unsigned int has_siblings = (ancestors_info[i] & 2) > 0;

            fprintf(file, "%s   ", has_siblings == 1 ? "|" : " ");
        }
        fprintf(file, "|\n");
        for (int i = 0; i < depth - 1; ++i) {
            const unsigned int has_siblings = (ancestors_info[i] & 2) > 0;

            fprintf(file, "%s   ", has_siblings == 1 ? "|" : " ");
        }
    }

	if (depth > 0) {
		fprintf(file, "+-> ");
	}

    switch (ast->kind) {
        case AST_NODE_KIND_EXPRESSION_STATEMENT:
            fprintf(file, "ExpressionStatement\n");
            if (ast->content.node != NULL)
                do_print_ast(ast->content.node, file, depth + 1, ancestors_info);
            break;
        case AST_NODE_KIND_COMPOUND_STATEMENT:
            {
                fprintf(file, "CompoundStatement\n");
                struct ast_node_list * iterator = ast->content.list;
                while (iterator != NULL) {
                    ancestors_info[depth] = iterator->next == NULL ? 0 : 2;
                    do_print_ast(iterator->node, file, depth + 1, ancestors_info);
                    iterator = iterator->next;
                }
            }
            break;
        case AST_NODE_KIND_EQUALITY_EXPRESSION:
            fprintf(file, "EqualityExpression: '%s'\n", ast->content.binary_expression.operation == BINARY_OPERATION_EQUALITY ? "==" : "!=");
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info);
            break;
        case AST_NODE_KIND_RELATIONAL_EXPRESSION:
            fprintf(file, "RelationalExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN ? '<' : '>');
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info);
            break;
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
            fprintf(file, "AdditiveExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_ADDITION ? '+' : '-');
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info);
            break;
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
            fprintf(file, "MultiplicativeExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_MULTIPLY ? '*' : '/');
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info);
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
            fprintf(file, "IntegerConstant: '%llu'\n", ast->content.integer_constant);
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            fprintf(file, "VariableDeclaration {name: '%s', type: 'int'}\n", ast->content.variable->name);
            break;
        case AST_NODE_KIND_FUNCTION_DEFINITION:
            fprintf(file, "FunctionDefinition: {name: '%s', type: 'int'}\n", ast->content.function_definition.name->name);
            if (ast->content.function_definition.body != NULL)
                do_print_ast(ast->content.function_definition.body, file, depth + 1, ancestors_info);
            break;
        default:
            fprintf(file, "<Unknown Ast Node>\n");
    }
}
