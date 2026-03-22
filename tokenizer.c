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


struct token eos_token = {NULL, NULL, &eos_token, {0, 0}, 0, TOKEN_KIND_EOS};

static void read_identifier(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch, uint32_t span_start);
static void read_number(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch, uint32_t span_start);
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

    token->source = source;
    token->identifier = NULL;

    for (;;) {
        int ch = source_get_char(source);

        while (isspace(ch)) {
            ch = source_get_char(source);
        }

        if (ch == EOF) {
            token->kind = TOKEN_KIND_EOS;
            token->span.offset = 0;
            token->span.length = 0;
            return;
        }

        uint32_t span_start = (uint32_t)(source->cursor - 1);

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
            token->span.offset = span_start;
            token->span.length = 1;
            return;
        }

        if (is_start_identifier_char(ch)) {
            read_identifier(ctx, source, token, ch, span_start);
            return;
        }

        if (ch == '.') {
            int next_char = source_get_char(source);
            if (isdigit(next_char)) {
                source_unget_char(source, next_char);
                read_number(ctx, source, token, ch, span_start);
                return;
            }
            source_unget_char(source, next_char);
        }

        if (isdigit(ch)) {
            read_number(ctx, source, token, ch, span_start);
            return;
        }

        switch (ch) {
            case '=':
                {
                    int next_char = source_get_char(source);

                    if (next_char == '=') {
                        token->kind = TOKEN_KIND_EQUAL_PUNCTUATOR;
                        token->span.offset = span_start;
                        token->span.length = 2;
                    } else {
                        source_unget_char(source, next_char);

                        token->kind = TOKEN_KIND_PUNCTUATOR;
                        token->span.offset = span_start;
                        token->span.length = 1;
                    }
                }
                break;
            case '!':
                {
                    int next_char = source_get_char(source);

                    if (next_char == '=') {
                        token->kind = TOKEN_KIND_NOT_EQUAL_PUNCTUATOR;
                        token->span.offset = span_start;
                        token->span.length = 2;
                    } else {
                        source_unget_char(source, next_char);

                        token->kind = TOKEN_KIND_PUNCTUATOR;
                        token->span.offset = span_start;
                        token->span.length = 1;
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
            case ',':
                token->kind = TOKEN_KIND_PUNCTUATOR;
                token->span.offset = span_start;
                token->span.length = 1;
                break;
            default:
                token->kind = TOKEN_KIND_UNKNOWN_CHARACTER;
                token->span.offset = span_start;
                token->span.length = 1;
        }

        return;
    }
}

void read_number(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch, uint32_t span_start)
{
    assert(ctx != NULL);
    assert(source != NULL);
    assert(token != NULL);

    for (;;) {
        ch = source_get_char(source);

        if (!isdigit(ch) && ch != '.') {
            if (ch != EOF) source_unget_char(source, ch);
            break;
        }
    }

    token->kind = TOKEN_KIND_NUMBER;
    token->span.offset = span_start;
    token->span.length = (uint32_t)(source->cursor - span_start);

    size_t dot_count = 0;
    const char * ptr = source->content + span_start;
    for (uint32_t i = 0; i < token->span.length; ++i) {
        if (ptr[i] == '.')
            ++dot_count;
    }

    if (dot_count > 1) {
        cclynx_fatal_error("ERROR: incorrect number '%.*s'\n", token->span.length, ptr);
    }

    if (dot_count > 0) {
        token->flags |= TOKEN_FLAG_IS_FLOAT;
    }
}

void read_identifier(struct tokenizer_context * ctx, struct source * source, struct token * token, int ch, uint32_t span_start)
{
    assert(ctx != NULL);
    assert(source != NULL);
    assert(token != NULL);

    do {
        ch = source_get_char(source);
    }
    while(ch != EOF && (is_identifier_char(ch) || isdigit(ch)));
    if (ch != EOF) source_unget_char(source, ch);

    token->span.offset = span_start;
    token->span.length = (uint32_t)(source->cursor - span_start);

    const char * ptr = source->content + span_start;
    uint32_t len = token->span.length;

    struct identifier * identifier = identifier_lookup(ctx->identifier_table, ptr, len);

    if (identifier == NULL) {
        identifier = identifier_insert(ctx->identifier_table, ctx->pool, ptr, len);
    }

    token->kind = TOKEN_KIND_IDENTIFIER;
    token->identifier = identifier;
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
