#ifndef TOKENIZER_H
#define TOKENIZER_H 1

#include <stdio.h>

#define is_upper_char(ch) ((ch) >= 'A' && (ch) <= 'Z')
#define is_lower_char(ch) ((ch) >= 'a' && (ch) <= 'z')
#define is_char(ch) (is_lower_char(ch) || is_upper_char(ch))
#define is_start_identifier_char(ch) (is_char(ch))
#define is_identifier_char(ch) (is_char(ch) || (ch) == '_')

#define TOKEN_FLAG_IS_FLOAT (1 << 0)

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
    union {
        int ch;
        struct identifier * identifier;
        char * number;
    } content;
    struct token * next;
    unsigned int flags;
    unsigned int kind;
};

extern struct token eos_token;

void tokenizer_get_one_token(FILE * file, struct token * token);
struct token * tokenizer_tokenize_file(FILE * file);

#endif /* TOKENIZER_H */
