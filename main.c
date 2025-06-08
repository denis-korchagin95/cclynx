#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "allocator.h"
#include "identifier.h"
#include "symbol.h"

#define MAX_CHAR_BUFFER_SIZE (4)
#define MAX_IDENTIFIER_BUFFER_SIZE (512)


#define TOKEN_KIND_EOF (0)
#define TOKEN_KIND_UNKNOWN_CHARACTER (1)
#define TOKEN_KIND_IDENTIFIER (2)
#define TOKEN_KIND_NUMBER (3)
#define TOKEN_KIND_KEYWORD (4)


#define TOKEN_KIND_PUNCTUATOR (1000)
#define TOKEN_KIND_SPECIAL_PUNCTUATOR (1001)


#define is_upper_char(ch) ((ch) >= 'A' && (ch) <= 'Z')
#define is_lower_char(ch) ((ch) >= 'a' && (ch) <= 'z')
#define is_char(ch) (is_lower_char(ch) || is_upper_char(ch))
#define is_start_identifier_char(ch) (is_char(ch))
#define is_identifier_char(ch) (is_char(ch) || (ch) == '_')

struct token
{
    unsigned int kind;
    union {
        int ch;
        struct identifier * identifier;
    } content;
};

static char char_buffer[MAX_CHAR_BUFFER_SIZE] = {0};
static unsigned int char_buffer_pos = 0;

static char identifier_buffer[MAX_IDENTIFIER_BUFFER_SIZE] = {0};
static unsigned int identifier_buffer_pos = 0;

static struct token * read_token(FILE * file);
static void print_token(struct token * token, FILE * file);

static int get_one_char(FILE * file);
static void putback_one_char(int ch);

struct memory_blob_pool main_pool = {0};

int main(int argc, const char * argv[])
{
    FILE * file = fopen("./examples/factorial.c", "r");

    memory_blob_pool_init(&main_pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
    init_builtin_symbols();

    for (;;) {
        struct token * token = read_token(file);

        print_token(token, stdout);

        if (token->kind == TOKEN_KIND_EOF) {
            break;
        }
    }

    memory_blob_pool_free(&main_pool, false);

    fclose(file);

    return 0;
}

void read_identifier(FILE * file, struct token * token, int ch)
{
    unsigned int hash = 0;

    identifier_buffer_pos = 0;

    while(ch != EOF && is_identifier_char(ch) && identifier_buffer_pos < MAX_IDENTIFIER_BUFFER_SIZE - 1) {
        identifier_buffer[identifier_buffer_pos++] = ch;
        hash = ch + 31 * hash;
        ch = get_one_char(file);
    }
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

struct token * read_token(FILE * file)
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