#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "tokenizer.h"
#include "allocator.h"
#include "symbol.h"
#include "identifier.h"

static char char_buffer[MAX_CHAR_BUFFER_SIZE] = {0};
static unsigned int char_buffer_pos = 0;

static char identifier_buffer[MAX_IDENTIFIER_BUFFER_SIZE] = {0};
static unsigned int identifier_buffer_pos = 0;

static int get_one_char(FILE * file);
static void putback_one_char(int ch);
static void read_identifier(FILE * file, struct token * token, int ch);

struct token * tokenizer_get_one_token(FILE * file)
{
    assert(file != NULL);

    struct token * token = (struct token *) memory_blob_pool_alloc(&main_pool, sizeof(struct token));

    int ch = get_one_char(file);

    while (isspace(ch)) {
        ch = get_one_char(file);
    }

    if (ch == EOF) {
        token->kind = TOKEN_KIND_EOF;
        token->content.ch = EOF;
        return token;
    }

    if (is_start_identifier_char(ch)) {
        read_identifier(file, token, ch);
        struct symbol * symbol = symbol_lookup(token->content.identifier, SYMBOL_KIND_KEYWORD);
        if (symbol != NULL) {
            token->kind = TOKEN_KIND_KEYWORD;
        }
        return token;
    }

    if (isdigit(ch)) {
        token->kind = TOKEN_KIND_NUMBER;
        int number = 0;
        for (;;) {
            number = number * 10 + (ch - '0');

            ch = get_one_char(file);

            if (!isdigit(ch)) {
                putback_one_char(ch);
                break;
            }
        }
        token->content.ch = number;
        return token;
    }

    switch (ch) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case '{':
        case '}':
        case '=':
        case ';':
        case '>':
        case '<':
        case '!':
            token->kind = TOKEN_KIND_PUNCTUATOR;
            token->content.ch = ch;
            break;
        default:
            token->kind = TOKEN_KIND_UNKNOWN_CHARACTER;
            token->content.ch = ch;
    }

    return token;
}

void read_identifier(FILE * file, struct token * token, int ch)
{
    unsigned int hash = 0;

    identifier_buffer_pos = 0;

    do {
        identifier_buffer[identifier_buffer_pos++] = ch;
        hash = ch + 31 * hash;
        ch = get_one_char(file);
    }
    while(ch != EOF && (is_identifier_char(ch) || isdigit(ch)) && identifier_buffer_pos < MAX_IDENTIFIER_BUFFER_SIZE - 1);
    identifier_buffer[identifier_buffer_pos] = '\0';
    putback_one_char(ch);

    if(identifier_buffer_pos >= MAX_IDENTIFIER_BUFFER_SIZE - 1) {
        fprintf(stderr, "warning: identifier too long!\n");
        while(is_identifier_char(ch))
            ch = get_one_char(file);
        putback_one_char(ch);
    }

    struct identifier * identifier = identifier_lookup(hash, identifier_buffer);

    if (identifier == NULL) {
        identifier = identifier_insert(hash, identifier_buffer, identifier_buffer_pos);
    }

    token->kind = TOKEN_KIND_IDENTIFIER;
    token->content.identifier = identifier;
}

int get_one_char(FILE * file)
{
    if (char_buffer_pos > 0) {
        return char_buffer[--char_buffer_pos];
    }

    return fgetc(file);
}

void putback_one_char(int ch)
{
    assert(char_buffer_pos < MAX_CHAR_BUFFER_SIZE);
    if (ch != EOF)
        char_buffer[char_buffer_pos++] = ch;
}
