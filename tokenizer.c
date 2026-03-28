#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "tokenizer.h"
#include "allocator.h"
#include "hashmap.h"
#include "identifier.h"
#include "errors.h"
#include "source.h"

#define NUMBER_TABLE_SIZE (101)


struct token eos_token = {{0}, &eos_token, 0, TOKEN_KIND_EOS};

static void read_identifier(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch);
static void read_number(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch);
static void skip_single_line_comment(struct tokenizer_context * ctx, struct source * source);
static void skip_multi_line_comment(struct tokenizer_context * ctx, struct source * source);


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

struct token * tokenizer_tokenize_file(struct tokenizer_context * ctx, struct source * source)
{
    assert(ctx != NULL);
    assert(source != NULL);

    struct token * tokens = &eos_token;
    struct token ** next_token = &tokens;

    for (;;) {
        struct token temp;
        memset(&temp, 0, sizeof(struct token));

        tokenizer_get_one_token(ctx, source, &temp);

        if (temp.kind == TOKEN_KIND_EOS) {
            *next_token = &eos_token;
            break;
        }

        struct token * token = memory_blob_pool_alloc(ctx->pool, sizeof(struct token));
        *token = temp;
        token->next = &eos_token;
        *next_token = token;
        next_token = &token->next;
    }

    return tokens;
}


void tokenizer_get_one_token(struct tokenizer_context * ctx, struct source * source, struct token * token)
{
    assert(ctx != NULL);
    assert(source != NULL);
    assert(token != NULL);

    for (;;) {
        int ch = source_get_char(source);

        while (isspace(ch)) {
            ch = source_get_char(source);
        }

        if (ch == EOF) {
            token->kind = TOKEN_KIND_EOS;
            token->content.ch = EOF;
            return;
        }

        if (ch == '/') {
            int next_char = source_get_char(source);

            if (next_char == '/') {
                skip_single_line_comment(ctx, source);
                continue;
            }

            if (next_char == '*') {
                skip_multi_line_comment(ctx, source);
                continue;
            }

            source_unget_char(source, next_char);
            token->kind = TOKEN_KIND_PUNCTUATOR;
            token->content.ch = ch;
            return;
        }

        if (is_start_identifier_char(ch)) {
            read_identifier(ctx, source, token, ch);
            return;
        }

        if (ch == '.') {
            int next_char = source_get_char(source);
            if (isdigit(next_char)) {
                source_unget_char(source, next_char);
                read_number(ctx, source, token, ch);
                return;
            }
            source_unget_char(source, next_char);
        }

        if (isdigit(ch)) {
            read_number(ctx, source, token, ch);
            return;
        }

        switch (ch) {
            case '=':
                {
                    int next_char = source_get_char(source);

                    if (next_char == '=') {
                        token->kind = TOKEN_KIND_EQUAL_PUNCTUATOR;
                    } else {
                        source_unget_char(source, next_char);

                        token->kind = TOKEN_KIND_PUNCTUATOR;
                        token->content.ch = ch;
                    }
                }
                break;
            case '!':
                {
                    int next_char = source_get_char(source);

                    if (next_char == '=') {
                        token->kind = TOKEN_KIND_NOT_EQUAL_PUNCTUATOR;
                    } else {
                        source_unget_char(source, next_char);

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

void read_number(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch)
{
    assert(ctx != NULL);
    assert(source != NULL);
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

        ch = source_get_char(source);

        if (!isdigit(ch) && ch != '.') {
            if (ch != EOF) source_unget_char(source, ch);
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

void read_identifier(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch)
{
    assert(ctx != NULL);
    assert(source != NULL);
    assert(token != NULL);

    ctx->identifier_buffer_pos = 0;

    do {
        ctx->identifier_buffer[ctx->identifier_buffer_pos++] = ch;
        ch = source_get_char(source);
    }
    while(ch != EOF && (is_identifier_char(ch) || isdigit(ch)) && ctx->identifier_buffer_pos < TOKENIZER_MAX_IDENTIFIER_BUFFER_SIZE - 1);
    ctx->identifier_buffer[ctx->identifier_buffer_pos] = '\0';
    if (ch != EOF) source_unget_char(source, ch);

    if(ctx->identifier_buffer_pos >= TOKENIZER_MAX_IDENTIFIER_BUFFER_SIZE - 1) {
        fprintf(stderr, "warning: identifier too long!\n");
        while(is_identifier_char(ch))
            ch = source_get_char(source);
        if (ch != EOF) source_unget_char(source, ch);
    }

    struct identifier * identifier = identifier_lookup(ctx->identifier_table, ctx->identifier_buffer);

    if (identifier == NULL) {
        identifier = identifier_insert(ctx->identifier_table, ctx->pool, ctx->identifier_buffer, ctx->identifier_buffer_pos);
    }

    token->kind = TOKEN_KIND_IDENTIFIER;
    token->content.identifier = identifier;
}

void skip_single_line_comment(struct tokenizer_context * ctx, struct source * source)
{
    assert(ctx != NULL);
    (void)ctx;
    int ch;
    do {
        ch = source_get_char(source);
    } while (ch != '\n' && ch != EOF);
}

void skip_multi_line_comment(struct tokenizer_context * ctx, struct source * source)
{
    assert(ctx != NULL);
    (void)ctx;
    for (;;) {
        int ch = source_get_char(source);

        if (ch == EOF) {
            cclynx_fatal_error("ERROR: unterminated comment\n");
        }

        if (ch == '*') {
            int next = source_get_char(source);
            if (next == '/') {
                return;
            }
            source_unget_char(source, next);
        }
    }
}
