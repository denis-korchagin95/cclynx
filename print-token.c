#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "print-token.h"

#include "tokenizer.h"
#include "identifier.h"

void print_token(const struct token * token, FILE * file)
{
    assert(token != NULL);
    assert(file != NULL);

    switch (token->kind) {
        case TOKEN_KIND_PUNCTUATOR:
            fprintf(file, "<TOKEN_PUNCTUATOR '%c'>\n", token_first_ch(token));
            break;
        case TOKEN_KIND_EOS:
            fprintf(file, "<TOKEN_EOS>\n");
            break;
        case TOKEN_KIND_UNKNOWN_CHARACTER:
            fprintf(file, "<TOKEN_UNKNOWN_CHARACTER '%c'>\n", token_first_ch(token));
            break;
        case TOKEN_KIND_IDENTIFIER:
            fprintf(file, "<TOKEN_%s '%s'>\n", token->identifier->keyword_code != KEYWORD_NONE ? "KEYWORD" : "IDENTIFIER", token->identifier->name);
            break;
        case TOKEN_KIND_NUMBER:
            fprintf(file, "<TOKEN_NUMBER '%.*s'>\n", token->span.length, token->source->content + token->span.offset);
            break;
        case TOKEN_KIND_EQUAL_PUNCTUATOR:
            fprintf(file, "<TOKEN_SPECIAL_PUNCTUATOR '=='>\n");
            break;
        case TOKEN_KIND_NOT_EQUAL_PUNCTUATOR:
            fprintf(file, "<TOKEN_SPECIAL_PUNCTUATOR '!='>\n");
            break;
        default:
            fprintf(file, "<TOKEN_UNKNOWN>\n");
            exit(1);
    }

    fflush(file);
}
