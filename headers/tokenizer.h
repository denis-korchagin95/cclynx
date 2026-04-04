#ifndef CCLYNX_TOKENIZER_H
#define CCLYNX_TOKENIZER_H 1

#include "source.h"

struct hashmap;

#define is_upper_char(ch) ((ch) >= 'A' && (ch) <= 'Z')
#define is_lower_char(ch) ((ch) >= 'a' && (ch) <= 'z')
#define is_char(ch) (is_lower_char(ch) || is_upper_char(ch))
#define is_start_identifier_char(ch) (is_char(ch))
#define is_identifier_char(ch) (is_char(ch) || (ch) == '_')

#define TOKEN_FLAG_IS_UNSIGNED (1 << 0)

#define token_first_ch(token) ((token)->source->content[(token)->span.offset])

#define token_is_identifier(token) \
    ((token)->kind == TOKEN_KIND_IDENTIFIER && (token)->identifier->keyword_code == KEYWORD_NONE)

#define token_is_keyword(token) \
    ((token)->kind == TOKEN_KIND_IDENTIFIER && (token)->identifier->keyword_code != KEYWORD_NONE)

#define token_is_punctuator(token, ch) \
    ((token)->kind == TOKEN_KIND_PUNCTUATOR && token_first_ch(token) == (ch))

struct identifier;

enum token_kind
{
    TOKEN_KIND_EOS = -1,
    TOKEN_KIND_UNKNOWN_CHARACTER = 0,
    TOKEN_KIND_IDENTIFIER,
    TOKEN_KIND_NUMBER,

    TOKEN_KIND_PUNCTUATOR = 1000,
    TOKEN_KIND_SPECIAL_PUNCTUATOR = 1001,
    TOKEN_KIND_EQUAL_PUNCTUATOR,
    TOKEN_KIND_NOT_EQUAL_PUNCTUATOR,
};

struct token
{
    struct source * source;
    struct identifier * identifier;
    struct token * next;
    struct source_span span;
    unsigned int flags;
    int kind;
};

extern struct token eos_token;

struct memory_blob_pool;

struct tokenizer_context {
    struct memory_blob_pool * pool;
    struct hashmap * identifier_table;
};

void tokenizer_init(struct tokenizer_context * ctx, struct hashmap * identifier_table, struct memory_blob_pool * pool);
void tokenizer_get_one_token(struct tokenizer_context * ctx, struct source * source, struct token * token);
struct token * tokenizer_tokenize_file(struct tokenizer_context * ctx, struct source * source);
const char * token_stringify(const struct token * token);

#endif /* CCLYNX_TOKENIZER_H */
