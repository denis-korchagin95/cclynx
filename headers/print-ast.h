#ifndef CCLYNX_PRINT_AST_H
#define CCLYNX_PRINT_AST_H 1

#include <stdio.h>

struct ast_node;

void print_ast(const struct ast_node * ast, FILE * file);

#endif /* CCLYNX_PRINT_AST_H */
