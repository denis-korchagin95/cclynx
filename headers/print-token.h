#ifndef CCLYNX_PRINT_TOKEN_H
#define CCLYNX_PRINT_TOKEN_H 1

#include <stdio.h>

struct token;

void print_token(const struct token * token, FILE * file);

#endif /* CCLYNX_PRINT_TOKEN_H */
