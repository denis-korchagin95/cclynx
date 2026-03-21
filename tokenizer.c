#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "allocator.h"
#include "hashmap.h"
#include "identifier.h"
#include "errors.h"

#define NUMBER_TABLE_SIZE (101)


struct token eos_token = {{0}, &eos_token, 0, TOKEN_KIND_EOS};

static int get_one_char(struct tokenizer_context * ctx, FILE * file);
static void putback_one_char(struct tokenizer_context * ctx, int ch);
static void read_identifier(struct tokenizer_context * ctx, FILE * file, struct token * token, int ch);
static void read_number(struct tokenizer_context * ctx, FILE * file, struct token * token, int ch);
static void skip_single_line_comment(struct tokenizer_context * ctx, FILE * file);
static void skip_multi_line_comment(struct tokenizer_context * ctx, FILE * file);


void tokenizer_init(struct tokenizer_context * ctx, struct hashmap * identifier_table, struct memory_blob_pool * pool)
{
    assert(ctx != NULL);
    assert(identifier_table != NULL);
    assert(pool != NULL);
    memset(ctx, 0, sizeof(struct tokenizer_context));
    ctx->pool = pool;
    ctx->identifier_table = identifier_table;
    hashmap_init(&ctx->number_table, NUMBER_TABLE_SIZE, pool);
}

struct token * tokenizer_tokenize_file(struct tokenizer_context * ctx, FILE * file)
{
    assert(ctx != NULL);
    assert(file != NULL);

    struct token * tokens = &eos_token;
    struct token ** next_token = &tokens;

    for (;;) {
        struct token * token = (struct token *) memory_blob_pool_alloc(ctx->pool, sizeof(struct token));
        memset(token, 0, sizeof(struct token));

        token->next = (*next_token)->next;
        *next_token = token;
        next_token = &token->next;

        tokenizer_get_one_token(ctx, file, token);

        if (token->kind == (unsigned int)TOKEN_KIND_EOS) {
            break;
        }
    }

    return tokens;
}


void tokenizer_get_one_token(struct tokenizer_context * ctx, FILE * file, struct token * token)
{
    assert(ctx != NULL);
    assert(file != NULL);
    assert(token != NULL);

    for (;;) {
        int ch = get_one_char(ctx, file);

        while (isspace(ch)) {
            ch = get_one_char(ctx, file);
        }

        if (ch == EOF) {
            token->kind = TOKEN_KIND_EOS;
            token->content.ch = EOF;
            return;
        }

        if (ch == '/') {
            int next_char = get_one_char(ctx, file);

            if (next_char == '/') {
                skip_single_line_comment(ctx, file);
                continue;
            }

            if (next_char == '*') {
                skip_multi_line_comment(ctx, file);
                continue;
            }

            putback_one_char(ctx, next_char);
            token->kind = TOKEN_KIND_PUNCTUATOR;
            token->content.ch = ch;
            return;
        }

        if (is_start_identifier_char(ch)) {
            read_identifier(ctx, file, token, ch);
            return;
        }

        if (ch == '.') {
            int next_char = get_one_char(ctx, file);
            if (isdigit(next_char)) {
                putback_one_char(ctx, next_char);
                read_number(ctx, file, token, ch);
                return;
            }
            putback_one_char(ctx, next_char);
        }

        if (isdigit(ch)) {
            read_number(ctx, file, token, ch);
            return;
        }

        switch (ch) {
            case '=':
                {
                    int next_char = get_one_char(ctx, file);

                    if (next_char == '=') {
                        token->kind = TOKEN_KIND_EQUAL_PUNCTUATOR;
                    } else {
                        putback_one_char(ctx, next_char);

                        token->kind = TOKEN_KIND_PUNCTUATOR;
                        token->content.ch = ch;
                    }
                }
                break;
            case '!':
                {
                    int next_char = get_one_char(ctx, file);

                    if (next_char == '=') {
                        token->kind = TOKEN_KIND_NOT_EQUAL_PUNCTUATOR;
                    } else {
                        putback_one_char(ctx, next_char);

                        token->kind = TOKEN_KIND_PUNCTUATOR;
                        token->content.ch = ch;
                    }
                }
                break;
            case '+':
            case '-':
            case '*':
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

        return;
    }
}

void read_number(struct tokenizer_context * ctx, FILE * file, struct token * token, int ch)
{
    assert(ctx != NULL);
    assert(file != NULL);
    assert(token != NULL);

    size_t dot_count = 0;
    ctx->number_buffer_pos = 0;
    for (;;) {
        if (ctx->number_buffer_pos >= TOKENIZER_MAX_NUMBER_BUFFER_SIZE - 1) {
            cclynx_fatal_error("ERROR: too long number!\n");
        }

        if (ch == '.')
            ++dot_count;

        ctx->number_buffer[ctx->number_buffer_pos++] = ch;

        ch = get_one_char(ctx, file);

        if (!isdigit(ch) && ch != '.') {
            putback_one_char(ctx, ch);
            break;
        }
    }
    ctx->number_buffer[ctx->number_buffer_pos] = '\0';

    if (dot_count > 1) {
        cclynx_fatal_error("ERROR: incorrect number '%s'\n", ctx->number_buffer);
    }

    char * number = hashmap_find(&ctx->number_table, ctx->number_buffer);

    if (number == NULL) {
        number = memory_blob_pool_alloc(ctx->pool, ctx->number_buffer_pos + 1);
        memcpy(number, ctx->number_buffer, ctx->number_buffer_pos + 1);
        hashmap_insert(&ctx->number_table, number, number);
    }

    token->kind = TOKEN_KIND_NUMBER;
    token->content.number = number;

    if (dot_count > 0) {
        token->flags |= TOKEN_FLAG_IS_FLOAT;
    }

}

void read_identifier(struct tokenizer_context * ctx, FILE * file, struct token * token, int ch)
{
    assert(ctx != NULL);
    assert(file != NULL);
    assert(token != NULL);

    ctx->identifier_buffer_pos = 0;

    do {
        ctx->identifier_buffer[ctx->identifier_buffer_pos++] = ch;
        ch = get_one_char(ctx, file);
    }
    while(ch != EOF && (is_identifier_char(ch) || isdigit(ch)) && ctx->identifier_buffer_pos < TOKENIZER_MAX_IDENTIFIER_BUFFER_SIZE - 1);
    ctx->identifier_buffer[ctx->identifier_buffer_pos] = '\0';
    putback_one_char(ctx, ch);

    if(ctx->identifier_buffer_pos >= TOKENIZER_MAX_IDENTIFIER_BUFFER_SIZE - 1) {
        fprintf(stderr, "warning: identifier too long!\n");
        while(is_identifier_char(ch))
            ch = get_one_char(ctx, file);
        putback_one_char(ctx, ch);
    }

    struct identifier * identifier = identifier_lookup(ctx->identifier_table, ctx->identifier_buffer);

    if (identifier == NULL) {
        identifier = identifier_insert(ctx->identifier_table, ctx->pool, ctx->identifier_buffer, ctx->identifier_buffer_pos);
    }

    token->kind = TOKEN_KIND_IDENTIFIER;
    token->content.identifier = identifier;
}

void skip_single_line_comment(struct tokenizer_context * ctx, FILE * file)
{
    assert(ctx != NULL);
    int ch;
    do {
        ch = get_one_char(ctx, file);
    } while (ch != '\n' && ch != EOF);
}

void skip_multi_line_comment(struct tokenizer_context * ctx, FILE * file)
{
    assert(ctx != NULL);
    for (;;) {
        int ch = get_one_char(ctx, file);

        if (ch == EOF) {
            cclynx_fatal_error("ERROR: unterminated comment\n");
        }

        if (ch == '*') {
            int next = get_one_char(ctx, file);
            if (next == '/') {
                return;
            }
            putback_one_char(ctx, next);
        }
    }
}

int get_one_char(struct tokenizer_context * ctx, FILE * file)
{
    assert(ctx != NULL);
    if (ctx->char_buffer_pos > 0) {
        return ctx->char_buffer[--ctx->char_buffer_pos];
    }

    return fgetc(file);
}

void putback_one_char(struct tokenizer_context * ctx, int ch)
{
    assert(ctx != NULL);
    assert(ctx->char_buffer_pos < TOKENIZER_MAX_CHAR_BUFFER_SIZE);
    if (ch != EOF)
        ctx->char_buffer[ctx->char_buffer_pos++] = ch;
}
