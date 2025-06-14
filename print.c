#include <assert.h>

#include "print.h"
#include "tokenizer.h"
#include "identifier.h"
#include "parser.h"

static void do_print_ast(struct ast_node * ast, FILE * file, int depth, unsigned int flags);

void print_token(struct token * token, FILE * file)
{
    assert(token != NULL);
    assert(file != NULL);

    switch (token->kind) {
        case TOKEN_KIND_KEYWORD:
            fprintf(file, "<TOKEN_KEYWORD '%s'>\n", token->content.identifier->name);
            break;
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
            fprintf(file, "<TOKEN_IDENTIFIER '%s'>\n", token->content.identifier->name);
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


void print_ast(struct ast_node * ast, FILE * file)
{
    assert(ast != NULL);
    assert(file != NULL);

	do_print_ast(ast, file, 0, 1);
}

void do_print_ast(struct ast_node * ast, FILE * file, int depth, unsigned int flags)
{
    const unsigned int is_top = (flags & 1) > 0;
    const unsigned int is_nested = (flags & 2) > 0;

    if (is_top == 0) {
        for (int i = 0; i < depth - 1; ++i) {
            fprintf(file, "%s   ", is_nested == 1 ? "|" : " ");
        }
        fprintf(file, "|\n");
        for (int i = 0; i < depth - 1; ++i) {
            fprintf(file, "%s   ", is_nested == 1 ? "|" : " ");
        }
    }

	if (depth > 0) {
		fprintf(file, "+-> ");
	}

    switch (ast->kind) {
        case AST_NODE_KIND_EQUALITY_EXPRESSION:
            fprintf(file, "EqualityExpression: '%s'\n", ast->content.binary_expression.operation == BINARY_OPERATION_EQUALITY ? "==" : "!=");
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, 2);
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, 2);
            break;
        case AST_NODE_KIND_RELATIONAL_EXPRESSION:
            fprintf(file, "RelationalExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN ? '<' : '>');
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, 2);
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, 2);
            break;
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
            fprintf(file, "AdditiveExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_ADDITION ? '+' : '-');
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, 2);
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, 2);
            break;
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
            fprintf(file, "MultiplicativeExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_MULTIPLY ? '*' : '/');
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, 2);
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, 2);
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
            fprintf(file, "IntegerConstant: '%llu'\n", ast->content.integer_constant);
            break;
        default:
            fprintf(file, "<Unknown Ast Node>\n");
    }
}
