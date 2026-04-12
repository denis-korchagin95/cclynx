#ifndef CCLYNX_PRINT_IR_H
#define CCLYNX_PRINT_IR_H 1

#include <stdio.h>

struct ir_program;

void print_ir_program(const struct ir_program * program, FILE * file);

#endif /* CCLYNX_PRINT_IR_H */
