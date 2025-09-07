#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "target-arm64.h"
#include "ir.h"
#include "identifier.h"
#include "util.h"
#include "type.h"

enum reg_kind
{
    REG_KIND_INTEGER,
    REG_KIND_FLOAT,
};

struct reg
{
    const char * name;
    enum reg_kind kind;
    unsigned int busy;
};

static struct reg regs[] = {
    { "w9",  REG_KIND_INTEGER,   0, },
    { "w10", REG_KIND_INTEGER,   0, },
    { "w11", REG_KIND_INTEGER,   0, },
    { "w12", REG_KIND_INTEGER,   0, },
    { "w13", REG_KIND_INTEGER,   0, },
    { "w14", REG_KIND_INTEGER,   0, },
    { "w15", REG_KIND_INTEGER,   0, },
    { "s8",  REG_KIND_FLOAT,     0, },
    { "s9",  REG_KIND_FLOAT,     0, },
    { "s10", REG_KIND_FLOAT,     0, },
    { "s11", REG_KIND_FLOAT,     0, },
    { "s12", REG_KIND_FLOAT,     0, },
    { "s13", REG_KIND_FLOAT,     0, },
    { "s14", REG_KIND_FLOAT,     0, },
    { "s15", REG_KIND_FLOAT,     0, },
};

static struct reg * reg_stack[16] = {NULL};
static unsigned int reg_stack_pos = 0;

static enum reg_kind get_reg_kind(struct type * type);

static void op_const(FILE * output, char * buf, struct ir_operand * op1);
static void op_load(FILE * output, struct ir_operand * op1);
static const char * reg_kind_stringify(enum reg_kind kind);

static void push_reg(struct reg * reg)
{
    if (reg_stack_pos >= 16) {
        fprintf(stderr, "ERROR: reg stack overflow for target arm64 generator\n");
        exit(1);
    }
    reg_stack[reg_stack_pos++] = reg;
}

static struct reg * pop_reg(void)
{
    if (reg_stack_pos <= 0) {
        fprintf(stderr, "ERROR: reg stack underflow for target arm64 generator\n");
        exit(1);
    }
    struct reg * reg = reg_stack[reg_stack_pos - 1];
    reg_stack[reg_stack_pos - 1] = NULL;
    --reg_stack_pos;
    return reg;
}

static struct reg * alloc_reg(enum reg_kind kind)
{
    size_t reg_count = sizeof(regs) / sizeof(struct reg);
    for (int i = 0; i < reg_count; ++i) {
        struct reg * reg = &regs[i];
        if (reg->kind == kind && reg->busy == 0) {
            reg->busy = 1;
            return reg;
        }
    }

    fprintf(stderr, "ERROR: too many registers\n");
    exit(1);
}

static void free_reg(struct reg * reg)
{
    assert(reg != NULL);

    reg->busy = 0;
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
            case OP_LABEL:
                fprintf(file, ".L%llu:\n", instruction->op1->content.label_id);
                break;
            case OP_JUMP:
                fprintf(file, "    b .L%llu\n", instruction->op1->content.label_id);
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
            case OP_NOP:
                fprintf(file, "    nop\n");
                break;
            case OP_STORE:
                {
                    struct reg * result_reg = pop_reg();
                    fprintf(file, "    str %s, [sp, #%zu]\n", result_reg->name, instruction->op1->content.variable.offset);
                    free_reg(result_reg);
                }
                break;
            case OP_JUMP_IF_FALSE:
                {
// TODO: implement float type support
                    struct reg * result_reg = pop_reg();
                    fprintf(file, "    cbz %s, .L%llu\n", result_reg->name, instruction->op2->content.label_id);
                    free_reg(result_reg);
                }
                break;
            case OP_JUMP_IF_EQUAL:
                {
// TODO: implement float type support
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    b.eq .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_NOT_EQUAL:
                {
// TODO: implement float type support
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    b.ne .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_LESS_OR_EQUAL:
                {
// TODO: implement float type support
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    b.le .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_GREATER_OR_EQUAL:
                {
// TODO: implement float type support
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    b.ge .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_IS_GREATER_THAN:
                {
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();

                    if (op1_reg->kind != op2_reg->kind) {
                        fprintf(stderr, "ERROR: register type mismatch (OP_IS_GREATER_THAN)!\n");
                        exit(1);
                    }

                    struct reg * result_reg = alloc_reg(REG_KIND_INTEGER);

                    if (op1_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, gt\n", result_reg->name);
                    } else if (op1_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, gt\n", result_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_IS_GREATER_THAN)!\n");
                        exit(1);
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_IS_LESS_THAN:
                {
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();

                    if (op1_reg->kind != op2_reg->kind) {
                        fprintf(stderr, "ERROR: register type mismatch (OP_IS_LESS_THAN)!\n");
                        exit(1);
                    }

                    struct reg * result_reg = alloc_reg(REG_KIND_INTEGER);

                    if (op1_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, lt\n", result_reg->name);
                    } else if (op1_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, lt\n", result_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_IS_LESS_THAN)!\n");
                        exit(1);
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_IS_EQUAL:
                {
// TODO: implement float type support
                    struct reg * result_reg = alloc_reg(REG_KIND_INTEGER);
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    cset %s, eq\n", result_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_IS_NOT_EQUAL:
                {
// TODO: implement float type support
                    struct reg * result_reg = alloc_reg(REG_KIND_INTEGER);
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    cset %s, ne\n", result_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_LOAD:
                op_load(file, instruction->op1);
                break;
            case OP_CONST:
                op_const(file, buf, instruction->op1);
                break;
            case OP_MUL:
                {
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();

                    if (op1_reg->kind != op2_reg->kind) {
                        fprintf(stderr, "ERROR: register type mismatch (OP_MUL)!\n");
                        exit(1);
                    }

                    struct reg * result_reg = alloc_reg(op1_reg->kind);

                    if (result_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    mul %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    fmul %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_MUL)!\n");
                        exit(1);
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_DIV:
                {
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();

                    if (op1_reg->kind != op2_reg->kind) {
                        fprintf(stderr, "ERROR: register type mismatch (OP_DIV)!\n");
                        exit(1);
                    }

                    struct reg * result_reg = alloc_reg(op1_reg->kind);

                    if (result_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    sdiv %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    fdiv %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_DIV)!\n");
                        exit(1);
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_SUB:
                {
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();

                    if (op1_reg->kind != op2_reg->kind) {
                        fprintf(stderr, "ERROR: register type mismatch (OP_SUB)!\n");
                        exit(1);
                    }

                    struct reg * result_reg = alloc_reg(op1_reg->kind);

                    if (result_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    sub %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    fsub %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_SUB)!\n");
                        exit(1);
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_ADD:
                {
                    struct reg * op2_reg = pop_reg();
                    struct reg * op1_reg = pop_reg();

                    if (op1_reg->kind != op2_reg->kind) {
                        fprintf(stderr, "ERROR: register type mismatch (OP_ADD)!\n");
                        exit(1);
                    }

                    struct reg * result_reg = alloc_reg(op1_reg->kind);

                    if (result_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    add %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    fadd %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_ADD)!\n");
                        exit(1);
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_RETURN:
                {
                    struct reg * result_reg = pop_reg();

                    if (result_reg->kind == REG_KIND_INTEGER) {
                        fprintf(file, "    mov w0, %s\n", result_reg->name);
                    } else if (result_reg->kind == REG_KIND_FLOAT) {
                        fprintf(file, "    mov s0, %s\n", result_reg->name);
                    } else {
                        fprintf(stderr, "ERROR: unsupported reg kind (OP_RETURN)!\n");
                        exit(1);
                    }

                    free_reg(result_reg);
                }
                break;
            case OP_INT_CAST:
                {
                    struct reg * op_reg = pop_reg();

                    struct reg * result_reg = NULL;

                    if (op_reg->kind != REG_KIND_FLOAT) {
                        fprintf(stderr, "ERROR: unsupported reg kind '%s' for OP_INT_CAST operation!\n", reg_kind_stringify(op_reg->kind));
                        exit(1);
                    }

                    result_reg = alloc_reg(REG_KIND_INTEGER);

                    fprintf(file, "    fcvtzs %s, %s\n", result_reg->name, op_reg->name);

                    free_reg(op_reg);
                    push_reg(result_reg);
                }
                break;
            case OP_FLOAT_CAST:
                {
                    struct reg * op_reg = pop_reg();

                    struct reg * result_reg = NULL;

                    if (op_reg->kind != REG_KIND_INTEGER) {
                        fprintf(stderr, "ERROR: unsupported reg kind '%s' for OP_FLOAT_CAST operation!\n", reg_kind_stringify(op_reg->kind));
                        exit(1);
                    }

                    result_reg = alloc_reg(REG_KIND_FLOAT);

                    fprintf(file, "    scvtf %s, %s\n", result_reg->name, op_reg->name);

                    free_reg(op_reg);
                    push_reg(result_reg);
                }
                break;
            default:
                fprintf(stderr, "ERROR: unknown instruction\n");
                exit(1);
        }
    }

    fflush(file);
}

enum reg_kind get_reg_kind(struct type * type)
{
    assert(type != NULL);

    if (type->kind == TYPE_KIND_INTEGER)
        return REG_KIND_INTEGER;

    if (type->kind == TYPE_KIND_FLOAT)
        return REG_KIND_FLOAT;

    fprintf(stderr, "ERROR: unsupported type \"%s\" for target arm64 code generation!\n", type_stringify(type));
    exit(1);
}

void op_const(FILE * output, char * buf, struct ir_operand * op1)
{
    assert(output != NULL);
    assert(op1 != NULL);
    assert(op1->type != NULL);

    struct reg * result_reg = alloc_reg(get_reg_kind(op1->type));

    if (result_reg->kind == REG_KIND_INTEGER) {
        sprintf(buf, "%d", op1->content.int_value);
        fprintf(output, "    mov %s, #%s\n", result_reg->name, buf);
        push_reg(result_reg);
        return;
    }
    else if (result_reg->kind == REG_KIND_FLOAT) {
        sprintf(buf, "%f", op1->content.float_value);
        fprintf(output, "    fmov %s, #%s\n", result_reg->name, buf);
        push_reg(result_reg);
        return;
    }

    fprintf(stderr, "ERROR: unsupported reg kind (OP_CONST)!\n");
    exit(1);
}

void op_load(FILE * output, struct ir_operand * op1)
{
    assert(output != NULL);
    assert(op1 != NULL);
    assert(op1->type != NULL);

    struct reg * result_reg = alloc_reg(get_reg_kind(op1->type));
    fprintf(output, "    ldr %s, [sp, #%zu]\n", result_reg->name, op1->content.variable.offset);
    push_reg(result_reg);
}

const char * reg_kind_stringify(enum reg_kind kind)
{
    if (kind == REG_KIND_INTEGER)
        return "int";

    if (kind == REG_KIND_FLOAT)
        return "float";

    return "<unknown reg kind>";
}
