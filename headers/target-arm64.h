#ifndef CCLYNX_TARGET_ARM64_H
#define CCLYNX_TARGET_ARM64_H 1

#include <stdio.h>

#define CODEGEN_REG_COUNT (7)
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

enum codegen_stack_entry_kind
{
    CODEGEN_STACK_ENTRY_REAL = 0,
    CODEGEN_STACK_ENTRY_SPILLED,
};

struct codegen_stack_entry
{
    enum codegen_stack_entry_kind kind;
    union {
        struct codegen_reg * reg;
        unsigned int spill_slot;
    } content;
};

struct codegen_context
{
    struct codegen_reg regs[CODEGEN_REG_COUNT];
    struct codegen_stack_entry reg_stack[CODEGEN_REG_STACK_SIZE];
    unsigned int reg_stack_pos;
    unsigned int current_func_max_spills;
    size_t current_func_locals_size;
    unsigned char spill_slot_used[CODEGEN_REG_STACK_SIZE];
    FILE * output;
    char buf[CODEGEN_BUF_SIZE];
};

void codegen_context_init(struct codegen_context * ctx);
void target_arm64_generate(struct codegen_context * ctx, struct ir_program * program, FILE * file);

#endif /* CCLYNX_TARGET_ARM64_H */
