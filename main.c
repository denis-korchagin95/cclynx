#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "allocator.h"

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
        char * str;
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

struct memory_blob_pool pool = {0};

int main(int argc, const char * argv[])
{
    FILE * file = fopen("./examples/factorial.c", "r");

    memory_blob_pool_init(&pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);

    for (;;) {
        struct token * token = read_token(file);

        print_token(token, stdout);

        if (token->kind == TOKEN_KIND_EOF) {
            break;
        }
    }

    memory_blob_pool_free(&pool, false);

    fclose(file);

    return 0;
}

struct token * read_token(FILE * file)
{
    assert(file != NULL);

    struct token * token = (struct token *) memory_blob_pool_alloc(&pool, sizeof(struct token));

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
        token->kind = TOKEN_KIND_IDENTIFIER;
        identifier_buffer_pos = 0;
        while (is_identifier_char(ch)) {
            // TODO: handle max identifier length
            identifier_buffer[identifier_buffer_pos++] = ch;
            ch = get_one_char(file);
        }
        identifier_buffer[identifier_buffer_pos] = '\0';
        putback_one_char(ch);
        char * str = malloc(identifier_buffer_pos + 1);
        strncpy(str, identifier_buffer, identifier_buffer_pos);
        str[identifier_buffer_pos] = '\0';
        token->content.str = str;
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
            fprintf(file, "<TOKEN_IDENTIFIER '%s'>\n", token->content.str);
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