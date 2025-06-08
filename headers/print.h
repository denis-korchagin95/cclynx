#ifndef PRINT_H
#define PRINT_H 1

#include <stdio.h>

struct token;

void print_token(struct token * token, FILE * file);

#endif /* PRINT_H */
