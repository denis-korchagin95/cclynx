#ifndef TOKENIZER_H
#define TOKENIZER_H 1

#include <stdio.h>

#define MAX_CHAR_BUFFER_SIZE (4)
#define MAX_IDENTIFIER_BUFFER_SIZE (512)

#define is_upper_char(ch) ((ch) >= 'A' && (ch) <= 'Z')
#define is_lower_char(ch) ((ch) >= 'a' && (ch) <= 'z')
#define is_char(ch) (is_lower_char(ch) || is_upper_char(ch))
#define is_start_identifier_char(ch) (is_char(ch))
#define is_identifier_char(ch) (is_char(ch) || (ch) == '_')

struct identifier;

enum token_kind
{
    TOKEN_KIND_EOS = -1,
    TOKEN_KIND_UNKNOWN_CHARACTER = 0,
    TOKEN_KIND_IDENTIFIER,
    TOKEN_KIND_NUMBER,
    TOKEN_KIND_KEYWORD,

    TOKEN_KIND_PUNCTUATOR = 1000,
    TOKEN_KIND_SPECIAL_PUNCTUATOR = 1001,
};

struct token
{
    union {
        int ch;
        struct identifier * identifier;
        long long int integer_constant;
    } content;
    struct token * next;
    unsigned int kind;
};

extern struct token eos_token;

void tokenizer_get_one_token(FILE * file, struct token * token);
struct token * tokenizer_tokenize_file(FILE * file);

#endif /* TOKENIZER_H */
