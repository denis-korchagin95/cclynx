#include <assert.h>

#include "print.h"
#include "tokenizer.h"
#include "identifier.h"
#include "parser.h"


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
        default:
            fprintf(file, "<TOKEN_UNKNOWN>\n");
    }
}


void print_ast(struct ast_node * ast, FILE * file)
{
    assert(ast != NULL);
    assert(file != NULL);

    switch (ast->kind) {
        case AST_NODE_KIND_INTEGER_CONSTANT:
            fprintf(file, "IntegerConstant: '%llu'\n", ast->content.integer_constant);
            break;
    }
}