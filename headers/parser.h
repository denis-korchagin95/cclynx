#ifndef CCLYNX_PARSER_H
#define CCLYNX_PARSER_H 1

#include <stdbool.h>

#include "ast.h"
#include "error.h"
#include "warning.h"

#define MAX_TOKEN_BUFFER_SIZE (4)

struct memory_blob_pool;

struct parser_context
{
    struct memory_blob_pool * pool;
    struct token * tokens;
    struct token * iterator;
    struct token * token_buffer[MAX_TOKEN_BUFFER_SIZE];
    unsigned int token_buffer_pos;
    struct scope * current_scope;
    const char * source_filename;
    struct error_list errors;
    struct symbol * current_function;
    bool has_error;
    struct warning_flags warning_flags;
};

void parser_init_context(struct parser_context * ctx, struct token * tokens, struct memory_blob_pool * pool, struct scope * file_scope, const char * source_filename);
struct ast_node * parser_parse(struct parser_context * ctx);

struct token * parser_get_token(struct parser_context * ctx);
struct token * parser_peek_token(struct parser_context * ctx);
void parser_putback_token(struct token * token, struct parser_context * ctx);
void parser_report_warning(struct parser_context * ctx, enum warning_code code, const struct token * token, const char * fmt, ...);

#endif /* CCLYNX_PARSER_H */
