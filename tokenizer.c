#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "allocator.h"
#include "identifier.h"

#define MAX_CHAR_BUFFER_SIZE (4)
#define MAX_IDENTIFIER_BUFFER_SIZE (512)
#define MAX_NUMBER_BUFFER_SIZE (512)


struct token eos_token = {{0}, &eos_token, TOKEN_KIND_EOS};

static char char_buffer[MAX_CHAR_BUFFER_SIZE] = {0};
static size_t char_buffer_pos = 0;

static char identifier_buffer[MAX_IDENTIFIER_BUFFER_SIZE] = {0};
static size_t identifier_buffer_pos = 0;

static char number_buffer[MAX_NUMBER_BUFFER_SIZE] = {0};
static size_t number_buffer_pos = 0;

static int get_one_char(FILE * file);
static void putback_one_char(int ch);
static void read_identifier(FILE * file, struct token * token, int ch);
static void read_number(FILE * file, struct token * token, int ch);


struct token * tokenizer_tokenize_file(FILE * file)
{
    assert(file != NULL);

    struct token * tokens = &eos_token;
    struct token ** next_token = &tokens;

    for (;;) {
        main_pool_alloc(struct token, token)

        token->next = (*next_token)->next;
        *next_token = token;
        next_token = &token->next;

        tokenizer_get_one_token(file, token);

        if (token->kind == TOKEN_KIND_EOS) {
            break;
        }
    }

    return tokens;
}


void tokenizer_get_one_token(FILE * file, struct token * token)
{
    assert(file != NULL);
    assert(token != NULL);

    int ch = get_one_char(file);

    while (isspace(ch)) {
        ch = get_one_char(file);
    }

    if (ch == EOF) {
        token->kind = TOKEN_KIND_EOS;
        token->content.ch = EOF;
        return;
    }

    if (is_start_identifier_char(ch)) {
        read_identifier(file, token, ch);
        return;
    }

    if (ch == '.') {
        int next_char = get_one_char(file);
        if (isdigit(next_char)) {
            putback_one_char(next_char);
            read_number(file, token, ch);
            return;
        }
        putback_one_char(next_char);
    }

    if (isdigit(ch)) {
        read_number(file, token, ch);
        return;
    }

    switch (ch) {
        case '=':
            {
                int next_char = get_one_char(file);

                if (next_char == '=') {
                    token->kind = TOKEN_KIND_EQUAL_PUNCTUATOR;
                } else {
                    putback_one_char(next_char);

                    token->kind = TOKEN_KIND_PUNCTUATOR;
                    token->content.ch = ch;
                }
            }
            break;
        case '!':
            {
                int next_char = get_one_char(file);

                if (next_char == '=') {
                    token->kind = TOKEN_KIND_NOT_EQUAL_PUNCTUATOR;
                } else {
                    putback_one_char(next_char);

                    token->kind = TOKEN_KIND_PUNCTUATOR;
                    token->content.ch = ch;
                }
            }
            break;
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case '{':
        case '}':
        case ';':
        case '>':
        case '<':
            token->kind = TOKEN_KIND_PUNCTUATOR;
            token->content.ch = ch;
            break;
        default:
            token->kind = TOKEN_KIND_UNKNOWN_CHARACTER;
            token->content.ch = ch;
    }
}

void read_number(FILE * file, struct token * token, int ch)
{
    assert(file != NULL);
    assert(token != NULL);

    size_t dot_count = 0;
    number_buffer_pos = 0;
    for (;;) {
        if (number_buffer_pos >= MAX_NUMBER_BUFFER_SIZE - 1) {
            fprintf(stderr, "ERROR: too long number!\n");
            exit(1);
        }

        if (ch == '.')
            ++dot_count;

        number_buffer[number_buffer_pos++] = ch;

        ch = get_one_char(file);

        if (!isdigit(ch) && ch != '.') {
            putback_one_char(ch);
            break;
        }
    }
    number_buffer[number_buffer_pos] = '\0';

    if (dot_count > 1) {
        fprintf(stderr, "ERROR: incorrect number '%s'\n", number_buffer);
        exit(1);
    }

    // TODO: implement storing the same number only for 1 time

    char * number = memory_blob_pool_alloc(&main_pool, number_buffer_pos);
    memcpy(number, number_buffer, number_buffer_pos);

    token->kind = TOKEN_KIND_NUMBER;
    token->content.number = number;

    if (dot_count > 0) {
        token->flags |= TOKEN_FLAG_IS_FLOAT;
    }

}

void read_identifier(FILE * file, struct token * token, int ch)
{
    assert(file != NULL);
    assert(token != NULL);

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

    struct identifier * identifier = identifier_find(hash, identifier_buffer);

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
