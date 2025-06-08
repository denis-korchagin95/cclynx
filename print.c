#include <assert.h>

#include "print.h"
#include "tokenizer.h"
#include "identifier.h"

void print_token(struct token * token, FILE * file)
{
    assert(file != NULL);
    assert(token != NULL);

    switch (token->kind) {
        case TOKEN_KIND_KEYWORD:
            fprintf(file, "<TOKEN_KEYWORD '%s'>\n", token->content.identifier->name);
            break;
        case TOKEN_KIND_PUNCTUATOR:
            fprintf(file, "<TOKEN_PUNCTUATOR '%c'>\n", token->content.ch);
            break;
        case TOKEN_KIND_EOF:
            fprintf(file, "<TOKEN_EOF>\n");
            break;
        case TOKEN_KIND_UNKNOWN_CHARACTER:
            fprintf(file, "<TOKEN_UNKNOWN_CHARACTER '%c'>\n", token->content.ch);
            break;
        case TOKEN_KIND_IDENTIFIER:
            fprintf(file, "<TOKEN_IDENTIFIER '%s'>\n", token->content.identifier->name);
            break;
        case TOKEN_KIND_NUMBER:
            fprintf(file, "<TOKEN_NUMBER '%d'>\n", token->content.ch);
            break;
        default:
            fprintf(file, "<TOKEN_UNKNOWN>\n");
    }
}
