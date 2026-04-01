#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "target-arm64.h"
#include "ir.h"
#include "identifier.h"
#include "util.h"
#include "type.h"
#include "errors.h"

static const struct codegen_reg initial_regs[CODEGEN_REG_COUNT] = {
    { "w9",  CODEGEN_REG_KIND_INTEGER,   0, },
    { "w10", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w11", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w12", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w13", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w14", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w15", CODEGEN_REG_KIND_INTEGER,   0, },
    { "s8",  CODEGEN_REG_KIND_FLOAT,     0, },
    { "s9",  CODEGEN_REG_KIND_FLOAT,     0, },
    { "s10", CODEGEN_REG_KIND_FLOAT,     0, },
    { "s11", CODEGEN_REG_KIND_FLOAT,     0, },
    { "s12", CODEGEN_REG_KIND_FLOAT,     0, },
    { "s13", CODEGEN_REG_KIND_FLOAT,     0, },
    { "s14", CODEGEN_REG_KIND_FLOAT,     0, },
    { "s15", CODEGEN_REG_KIND_FLOAT,     0, },
};

static enum codegen_reg_kind get_reg_kind(struct type * type);

static void op_const(struct codegen_context * ctx, FILE * output, struct ir_operand * op1);
static void op_load(struct codegen_context * ctx, FILE * output, struct ir_operand * op1);
static const char * reg_kind_stringify(enum codegen_reg_kind kind);

static void push_reg(struct codegen_context * ctx, struct codegen_reg * reg)
{
    assert(ctx != NULL);
    assert(reg != NULL);
    if (ctx->reg_stack_pos >= CODEGEN_REG_STACK_SIZE) {
        cclynx_fatal_error("ERROR: reg stack overflow for target arm64 generator\n");
    }
    ctx->reg_stack[ctx->reg_stack_pos++] = reg;
}

static struct codegen_reg * pop_reg(struct codegen_context * ctx)
{
    assert(ctx != NULL);
    if (ctx->reg_stack_pos <= 0) {
        cclynx_fatal_error("ERROR: reg stack underflow for target arm64 generator\n");
    }
    struct codegen_reg * reg = ctx->reg_stack[ctx->reg_stack_pos - 1];
    ctx->reg_stack[ctx->reg_stack_pos - 1] = NULL;
    --ctx->reg_stack_pos;
    return reg;
}

static struct codegen_reg * alloc_reg(struct codegen_context * ctx, enum codegen_reg_kind kind)
{
    assert(ctx != NULL);
    for (size_t i = 0; i < CODEGEN_REG_COUNT; ++i) {
        struct codegen_reg * reg = &ctx->regs[i];
        if (reg->kind == kind && reg->busy == 0) {
            reg->busy = 1;
            return reg;
        }
    }

    cclynx_fatal_error("ERROR: too many registers\n");
}

static void free_reg(struct codegen_reg * reg)
{
    assert(reg != NULL);

    reg->busy = 0;
}

void codegen_context_init(struct codegen_context * ctx)
{
    assert(ctx != NULL);
    memset(ctx, 0, sizeof(struct codegen_context));
    memcpy(ctx->regs, initial_regs, sizeof(initial_regs));
}

void target_arm64_generate(struct codegen_context * ctx, struct ir_program * program, FILE * file)
{
    assert(ctx != NULL);
    assert(program != NULL);
    assert(file != NULL);
    assert(program->position > 0);

    fprintf(file, ".text\n");
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
                fprintf(file, ".global _%s\n", instruction->result->content.function.identifier->name);
                fprintf(file, "_%s:\n", instruction->result->content.function.identifier->name);
                fprintf(file, "    stp x29, x30, [sp, -16]!\n");
                fprintf(file, "    mov x29, sp\n");
                if (instruction->result->content.function.local_vars_size > 0) {
                    fprintf(file, "    sub sp, sp, #%zu\n", align_up(instruction->result->content.function.local_vars_size, 16));
                }
                break;
            case OP_FUNC_END:
                break;
            case OP_NOP:
                fprintf(file, "    nop\n");
                break;
            case OP_STORE:
                {
                    struct codegen_reg * result_reg = pop_reg(ctx);
                    fprintf(file, "    str %s, [sp, #%zu]\n", result_reg->name, instruction->op1->content.variable.offset);
                    free_reg(result_reg);
                }
                break;
            case OP_JUMP_IF_FALSE:
                {
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cbz %s, .L%llu\n", op1_reg->name, instruction->op2->content.label_id);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, #0.0\n", op1_reg->name);
                        fprintf(file, "    b.eq .L%llu\n", instruction->op2->content.label_id);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_JUMP_IF_FALSE)!\n");
                    }

                    free_reg(op1_reg);
                }
                break;
            case OP_JUMP_IF_EQUAL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_JUMP_IF_EQUAL)!\n");
                    }

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.eq .L%llu\n", instruction->result->content.label_id);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.eq .L%llu\n", instruction->result->content.label_id);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_JUMP_IF_EQUAL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_NOT_EQUAL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_JUMP_IF_NOT_EQUAL)!\n");
                    }

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.ne .L%llu\n", instruction->result->content.label_id);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.ne .L%llu\n", instruction->result->content.label_id);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_JUMP_IF_NOT_EQUAL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_LESS_OR_EQUAL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_JUMP_IF_LESS_OR_EQUAL)!\n");
                    }

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.le .L%llu\n", instruction->result->content.label_id);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.le .L%llu\n", instruction->result->content.label_id);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_JUMP_IF_LESS_OR_EQUAL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_GREATER_OR_EQUAL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_JUMP_IF_GREATER_OR_EQUAL)!\n");
                    }

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.ge .L%llu\n", instruction->result->content.label_id);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    b.ge .L%llu\n", instruction->result->content.label_id);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_JUMP_IF_GREATER_OR_EQUAL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_IS_GREATER_THAN:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_IS_GREATER_THAN)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, gt\n", result_reg->name);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, gt\n", result_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_IS_GREATER_THAN)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_IS_LESS_THAN:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_IS_LESS_THAN)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, lt\n", result_reg->name);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, lt\n", result_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_IS_LESS_THAN)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_IS_EQUAL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_IS_EQUAL)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, eq\n", result_reg->name);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, eq\n", result_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_IS_EQUAL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_IS_NOT_EQUAL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_IS_NOT_EQUAL)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);

                    if (op1_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, ne\n", result_reg->name);
                    } else if (op1_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fcmp %s, %s\n", op1_reg->name, op2_reg->name);
                        fprintf(file, "    cset %s, ne\n", result_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_IS_NOT_EQUAL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_LOAD:
                op_load(ctx, file, instruction->op1);
                break;
            case OP_CONST:
                op_const(ctx, file, instruction->op1);
                break;
            case OP_MUL:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_MUL)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, op1_reg->kind);

                    if (result_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    mul %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fmul %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_MUL)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_DIV:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_DIV)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, op1_reg->kind);

                    if (result_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    sdiv %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fdiv %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_DIV)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_SUB:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_SUB)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, op1_reg->kind);

                    if (result_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    sub %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fsub %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_SUB)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_ADD:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);

                    if (op1_reg->kind != op2_reg->kind) {
                        cclynx_fatal_error("ERROR: register type mismatch (OP_ADD)!\n");
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, op1_reg->kind);

                    if (result_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                        fprintf(file, "    add %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else if (result_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                        fprintf(file, "    fadd %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    } else {
                        cclynx_fatal_error("ERROR: unsupported reg kind (OP_ADD)!\n");
                    }

                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_RETURN:
                {
                    if (instruction->op1 != NULL) {
                        struct codegen_reg * result_reg = pop_reg(ctx);

                        if (result_reg->kind == CODEGEN_REG_KIND_INTEGER) {
                            fprintf(file, "    mov w0, %s\n", result_reg->name);
                        } else if (result_reg->kind == CODEGEN_REG_KIND_FLOAT) {
                            fprintf(file, "    fmov s0, %s\n", result_reg->name);
                        } else {
                            cclynx_fatal_error("ERROR: unsupported reg kind (OP_RETURN)!\n");
                        }

                        free_reg(result_reg);
                    }

                    if (instruction->result->content.function.local_vars_size > 0) {
                        fprintf(file, "    add sp, sp, #%zu\n", align_up(instruction->result->content.function.local_vars_size, 16));
                    }
                    fprintf(file, "    ldp x29, x30, [sp], #16\n");
                    fprintf(file, "    ret\n");
                }
                break;
            case OP_INT_CAST:
                {
                    struct codegen_reg * op_reg = pop_reg(ctx);

                    struct codegen_reg * result_reg = NULL;

                    if (op_reg->kind != CODEGEN_REG_KIND_FLOAT) {
                        cclynx_fatal_error("ERROR: unsupported reg kind '%s' for OP_INT_CAST operation!\n", reg_kind_stringify(op_reg->kind));
                    }

                    result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);

                    fprintf(file, "    fcvtzs %s, %s\n", result_reg->name, op_reg->name);

                    free_reg(op_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_FLOAT_CAST:
                {
                    struct codegen_reg * op_reg = pop_reg(ctx);

                    struct codegen_reg * result_reg = NULL;

                    if (op_reg->kind != CODEGEN_REG_KIND_INTEGER) {
                        cclynx_fatal_error("ERROR: unsupported reg kind '%s' for OP_FLOAT_CAST operation!\n", reg_kind_stringify(op_reg->kind));
                    }

                    result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_FLOAT);

                    fprintf(file, "    scvtf %s, %s\n", result_reg->name, op_reg->name);

                    free_reg(op_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_STORE_PARAM:
                {
                    size_t offset = instruction->op1->content.variable.offset;
                    int param_index = (int) instruction->op2->content.int_value;
                    fprintf(file, "    str w%d, [sp, #%zu]\n", param_index, offset);
                }
                break;
            case OP_ARG:
                {
                    struct codegen_reg * arg_reg = pop_reg(ctx);
                    int arg_index = (int) instruction->op2->content.int_value;
                    fprintf(file, "    mov w%d, %s\n", arg_index, arg_reg->name);
                    free_reg(arg_reg);
                }
                break;
            case OP_CALL:
                {
                    size_t saved_active_reg_count = ctx->reg_stack_pos;
                    size_t spill_memory_size = align_up(saved_active_reg_count * 4, 16);

                    if (saved_active_reg_count > 0) {
                        fprintf(file, "    sub sp, sp, #%zu\n", spill_memory_size);
                        for (size_t j = 0; j < saved_active_reg_count; ++j) {
                            fprintf(file, "    str %s, [sp, #%zu]\n", ctx->reg_stack[j]->name, j * 4);
                        }
                    }

                    fprintf(file, "    bl _%s\n", instruction->op1->content.function.identifier->name);

                    if (saved_active_reg_count > 0) {
                        for (size_t j = 0; j < saved_active_reg_count; ++j) {
                            fprintf(file, "    ldr %s, [sp, #%zu]\n", ctx->reg_stack[j]->name, j * 4);
                        }
                        fprintf(file, "    add sp, sp, #%zu\n", spill_memory_size);
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, get_reg_kind(instruction->result->type));
                    fprintf(file, "    mov %s, w0\n", result_reg->name);
                    push_reg(ctx, result_reg);
                }
                break;
            default:
                cclynx_fatal_error("ERROR: unknown instruction\n");
        }
    }

    fflush(file);
}

enum codegen_reg_kind get_reg_kind(struct type * type)
{
    assert(type != NULL);

    if (type->kind == TYPE_KIND_INTEGER)
        return CODEGEN_REG_KIND_INTEGER;

    if (type->kind == TYPE_KIND_FLOAT)
        return CODEGEN_REG_KIND_FLOAT;

    cclynx_fatal_error("ERROR: unsupported type \"%s\" for target arm64 code generation!\n", type_stringify(type));
}

void op_const(struct codegen_context * ctx, FILE * output, struct ir_operand * op1)
{
    assert(ctx != NULL);
    assert(output != NULL);
    assert(op1 != NULL);
    assert(op1->type != NULL);

    struct codegen_reg * result_reg = alloc_reg(ctx, get_reg_kind(op1->type));

    if (result_reg->kind == CODEGEN_REG_KIND_INTEGER) {
        unsigned int bits = (unsigned int) op1->content.int_value;
        unsigned int lo = bits & 0xFFFF;
        unsigned int hi = (bits >> 16) & 0xFFFF;

        if (hi == 0) {
            snprintf(ctx->buf, CODEGEN_BUF_SIZE, "%lld", op1->content.int_value);
            fprintf(output, "    mov %s, #%s\n", result_reg->name, ctx->buf);
        } else {
            fprintf(output, "    movz %s, #0x%x\n", result_reg->name, lo);
            fprintf(output, "    movk %s, #0x%x, lsl #16\n", result_reg->name, hi);
        }

        push_reg(ctx, result_reg);
        return;
    }
    else if (result_reg->kind == CODEGEN_REG_KIND_FLOAT) {
        unsigned int bits;
        memcpy(&bits, &op1->content.float_value, sizeof(bits));

        unsigned int lo = bits & 0xFFFF;
        unsigned int hi = (bits >> 16) & 0xFFFF;

        struct codegen_reg * tmp_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
        fprintf(output, "    movz %s, #0x%x\n", tmp_reg->name, lo);
        if (hi != 0) {
            fprintf(output, "    movk %s, #0x%x, lsl #16\n", tmp_reg->name, hi);
        }
        fprintf(output, "    fmov %s, %s\n", result_reg->name, tmp_reg->name);
        free_reg(tmp_reg);
        push_reg(ctx, result_reg);
        return;
    }

    cclynx_fatal_error("ERROR: unsupported reg kind (OP_CONST)!\n");
}

void op_load(struct codegen_context * ctx, FILE * output, struct ir_operand * op1)
{
    assert(ctx != NULL);
    assert(output != NULL);
    assert(op1 != NULL);
    assert(op1->type != NULL);

    struct codegen_reg * result_reg = alloc_reg(ctx, get_reg_kind(op1->type));
    fprintf(output, "    ldr %s, [sp, #%zu]\n", result_reg->name, op1->content.variable.offset);
    push_reg(ctx, result_reg);
}

const char * reg_kind_stringify(enum codegen_reg_kind kind)
{
    if (kind == CODEGEN_REG_KIND_INTEGER)
        return "int";

    if (kind == CODEGEN_REG_KIND_FLOAT)
        return "float";

    return "<unknown reg kind>";
}
