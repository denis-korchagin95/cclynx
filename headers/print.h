#ifndef PRINT_H
#define PRINT_H 1

#include <stdio.h>

struct token;
struct ast_node;
struct ir_program;

void print_token(const struct token * token, FILE * file);
void print_ast(const struct ast_node * ast, FILE * file);
void print_ir_program(const struct ir_program * program, FILE * file);

#endif /* PRINT_H */
