#ifndef CCLYNX_TARGET_ARM64_H
#define CCLYNX_TARGET_ARM64_H 1

#include <stdio.h>

#define CODEGEN_REG_COUNT (15)
#define CODEGEN_REG_STACK_SIZE (16)
#define CODEGEN_BUF_SIZE (1024)

struct ir_program;

enum codegen_reg_kind
{
    CODEGEN_REG_KIND_INTEGER,
};

struct codegen_reg
{
    const char * name;
    enum codegen_reg_kind kind;
    unsigned int busy;
};

struct codegen_context
{
    struct codegen_reg regs[CODEGEN_REG_COUNT];
    struct codegen_reg * reg_stack[CODEGEN_REG_STACK_SIZE];
    unsigned int reg_stack_pos;
    char buf[CODEGEN_BUF_SIZE];
};

void codegen_context_init(struct codegen_context * ctx);
void target_arm64_generate(struct codegen_context * ctx, struct ir_program * program, FILE * file);

#endif /* CCLYNX_TARGET_ARM64_H */
