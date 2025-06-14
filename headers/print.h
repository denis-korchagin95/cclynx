#ifndef PRINT_H
#define PRINT_H 1

#include <stdio.h>

struct token;
struct ast_node;

void print_token(const struct token * token, FILE * file);
void print_ast(const struct ast_node * ast, FILE * file);

#endif /* PRINT_H */
