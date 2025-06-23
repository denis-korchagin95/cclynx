#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "target-arm64.h"
#include "ir.h"
#include "identifier.h"
#include "util.h"

static unsigned int regs[5] = {0};
static const char * reg_names[] = {"w9", "w10", "w11", "w12", "w13", "w14", "w15"};
static unsigned int stack[16] = {0};
static unsigned int stack_pos = 0;

static void push_reg(unsigned int reg)
{
    if (stack_pos >= 16) {
        fprintf(stderr, "ERROR: reg stack overflow for target arm64 generator\n");
        exit(1);
    }
    stack[stack_pos++] = reg;
}

static unsigned int pop_reg(void)
{
    if (stack_pos <= 0) {
        fprintf(stderr, "ERROR: reg stack underflow for target arm64 generator\n");
        exit(1);
    }
    unsigned int reg = stack[stack_pos - 1];
    stack[stack_pos - 1] = 0;
    --stack_pos;
    return reg;
}

static unsigned int alloc_reg(void)
{
    for (int i = 0; i < 5; ++i) {
        if (regs[i] == 0) {
            regs[i] = 1;
            return i + 1;
        }
    }

    fprintf(stderr, "ERROR: too many registers\n");
    exit(1);
}

static void free_reg(unsigned int reg)
{
    if (reg >= 1 && reg <= 5) {
        regs[reg - 1] = 0;
        return;
    }

    fprintf(stderr, "ERROR: unknown register\n");
    exit(1);
}

static const char * get_reg_name(unsigned int reg)
{
    if (reg >= 1 && reg <= 5) {
        return reg_names[reg - 1];
    }

    fprintf(stderr, "ERROR: unknown register\n");
    exit(1);
}

void target_arm64_generate(struct ir_program * program, FILE * file)
{
    assert(program != NULL);
    assert(file != NULL);
    assert(program->position > 0);

    static char buf[1024] = {'\0'};

    fprintf(file, ".text\n");
    fprintf(file, ".global _main\n");
    fprintf(file, ".align 2\n");
    fprintf(file, "\n");

    for (size_t i = 0; i < program->position; ++i) {
        struct ir_instruction * instruction = program->instructions[i];

        switch (instruction->code) {
            case OP_JUMP:
                {
                    fprintf(file, "    b .L%llu\n", instruction->op1->content.label_id);
                }
                break;
            case OP_JUMP_IF_FALSE:
                {
                    unsigned int op_reg = pop_reg();
                    fprintf(file, "    cbz %s, .L%llu\n", get_reg_name(op_reg), instruction->op2->content.label_id);
                    free_reg(op_reg);
                }
                break;
            case OP_LABEL:
                {
                    fprintf(file, ".L%llu:\n", instruction->op1->content.label_id);
                }
                break;
            case OP_JUMP_IF_EQUAL:
                {
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    b.eq .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_NOT_EQUAL:
                {
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    b.ne .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_LESS_OR_EQUAL:
                {
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    b.le .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_GREATER_OR_EQUAL:
                {
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    b.ge .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_IS_GREATER_THAN:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    cset %s, gt\n", get_reg_name(op_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_IS_LESS_THAN:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    cset %s, lt\n", get_reg_name(op_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_IS_EQUAL:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    cset %s, eq\n", get_reg_name(op_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_IS_NOT_EQUAL:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", get_reg_name(op1_reg), get_reg_name(op2_reg));
                    fprintf(file, "    cset %s, ne\n", get_reg_name(op_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_FUNC:
                fprintf(file, "_%s:\n", instruction->result->content.function.identifier->name);
                fprintf(file, "    stp x29, x30, [sp, -16]!\n");
                fprintf(file, "    mov x29, sp\n");
                if (instruction->result->content.function.local_vars_size > 0) {
                    fprintf(file, "    sub sp, sp, #%zu\n", align_up(instruction->result->content.function.local_vars_size, 16));
                }
                break;
            case OP_FUNC_END:
                fprintf(file, "    add sp, sp, #16\n");
                fprintf(file, "    ldp x29, x30, [sp], #16\n");
                fprintf(file, "    ret\n");
                break;
            case OP_STORE:
                {
                    unsigned int op_reg = pop_reg();
                    fprintf(file, "    str %s, [sp, #%zu]\n", get_reg_name(op_reg), instruction->op1->content.variable.offset);
                    free_reg(op_reg);
                }
                break;
            case OP_LOAD:
                {
                    unsigned int op_reg = alloc_reg();
                    fprintf(file, "    ldr %s, [sp, #%zu]\n", get_reg_name(op_reg), instruction->op1->content.variable.offset);
                    push_reg(op_reg);
                }
                break;
            case OP_CONST:
                {
                    unsigned int op_reg = alloc_reg();
                    sprintf(buf, "%lli", instruction->op1->content.llic);
                    fprintf(file, "    mov %s, #%s\n", get_reg_name(op_reg), buf);
                    push_reg(op_reg);
                }
                break;
            case OP_MUL:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    mul %s, %s, %s\n", get_reg_name(op_reg), get_reg_name(op1_reg), get_reg_name(op2_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_DIV:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    sdiv %s, %s, %s\n", get_reg_name(op_reg), get_reg_name(op1_reg), get_reg_name(op2_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_SUB:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    sub %s, %s, %s\n", get_reg_name(op_reg), get_reg_name(op1_reg), get_reg_name(op2_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_ADD:
                {
                    unsigned int op_reg = alloc_reg();
                    unsigned int op2_reg = pop_reg();
                    unsigned int op1_reg = pop_reg();
                    fprintf(file, "    add %s, %s, %s\n", get_reg_name(op_reg), get_reg_name(op1_reg), get_reg_name(op2_reg));
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(op_reg);
                }
                break;
            case OP_RETURN:
                {
                    unsigned int op_reg = pop_reg();
                    fprintf(file, "    mov w0, %s\n", get_reg_name(op_reg));
                    free_reg(op_reg);
                }
                break;
            default:
                fprintf(stderr, "ERROR: unknown instruction\n");
                exit(1);
        }
    }

    fflush(file);
}
