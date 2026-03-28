#ifndef CCLYNX_TOKENIZER_H
#define CCLYNX_TOKENIZER_H 1

#include "hashmap.h"
#include "source.h"

#define is_upper_char(ch) ((ch) >= 'A' && (ch) <= 'Z')
#define is_lower_char(ch) ((ch) >= 'a' && (ch) <= 'z')
#define is_char(ch) (is_lower_char(ch) || is_upper_char(ch))
#define is_start_identifier_char(ch) (is_char(ch))
#define is_identifier_char(ch) (is_char(ch) || (ch) == '_')

#define TOKEN_FLAG_IS_FLOAT (1 << 0)

#define token_first_ch(tok) ((tok)->source->content[(tok)->span.offset])

#define token_is_punctuator(tok, c) \
    ((tok)->kind == TOKEN_KIND_PUNCTUATOR && token_first_ch(tok) == (c))

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

#define TOKENIZER_MAX_IDENTIFIER_BUFFER_SIZE (512)
#define TOKENIZER_MAX_NUMBER_BUFFER_SIZE (512)

struct memory_blob_pool;

struct tokenizer_context {
    struct memory_blob_pool * pool;
    struct hashmap * identifier_table;
    char identifier_buffer[TOKENIZER_MAX_IDENTIFIER_BUFFER_SIZE];
    size_t identifier_buffer_pos;
    char number_buffer[TOKENIZER_MAX_NUMBER_BUFFER_SIZE];
    size_t number_buffer_pos;
};

void tokenizer_init(struct tokenizer_context * ctx, struct hashmap * identifier_table, struct memory_blob_pool * pool);
void tokenizer_get_one_token(struct tokenizer_context * ctx, struct source * source, struct token * token);
struct token * tokenizer_tokenize_file(struct tokenizer_context * ctx, struct source * source);

#endif /* CCLYNX_TOKENIZER_H */
