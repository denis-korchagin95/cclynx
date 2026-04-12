#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "target-arm64.h"
#include "ir.h"
#include "identifier.h"
#include "util.h"
#include "type.h"
#include "error.h"

static const struct codegen_reg initial_regs[CODEGEN_REG_COUNT] = {
    { "w9",  CODEGEN_REG_KIND_INTEGER,   0, },
    { "w10", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w11", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w12", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w13", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w14", CODEGEN_REG_KIND_INTEGER,   0, },
    { "w15", CODEGEN_REG_KIND_INTEGER,   0, },
};

struct simulator_context
{
    unsigned int live_regs;
    unsigned int stack_pos;
    unsigned int stack_spilled_count;
    unsigned int max_spilled_live;
    unsigned char stack_is_spill[CODEGEN_REG_STACK_SIZE];
};

static void op_const(struct codegen_context * ctx, FILE * output, struct ir_operand * op1);
static void op_load(struct codegen_context * ctx, FILE * output, struct ir_operand * op1);

static void push_reg(struct codegen_context * ctx, struct codegen_reg * reg)
{
    assert(ctx != NULL);
    assert(reg != NULL);
    if (ctx->reg_stack_pos >= CODEGEN_REG_STACK_SIZE) {
        cclynx_fatal_error("ERROR: reg stack overflow for target arm64 generator\n");
    }
    struct codegen_stack_entry * entry = &ctx->reg_stack[ctx->reg_stack_pos++];
    entry->kind = CODEGEN_STACK_ENTRY_REAL;
    entry->content.reg = reg;
}

static unsigned int alloc_spill_slot(struct codegen_context * ctx)
{
    for (unsigned int i = 0; i < ctx->current_func_max_spills; ++i) {
        if (!ctx->spill_slot_used[i]) {
            ctx->spill_slot_used[i] = 1;
            return i;
        }
    }
    cclynx_fatal_error("ERROR: out of spill slots\n");
}

static void free_spill_slot(struct codegen_context * ctx, unsigned int slot)
{
    assert(slot < ctx->current_func_max_spills);
    assert(ctx->spill_slot_used[slot]);
    ctx->spill_slot_used[slot] = 0;
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

    for (unsigned int k = 0; k < ctx->reg_stack_pos; ++k) {
        struct codegen_stack_entry * entry = &ctx->reg_stack[k];
        if (entry->kind != CODEGEN_STACK_ENTRY_REAL) {
            continue;
        }
        struct codegen_reg * victim = entry->content.reg;
        if (victim->kind != kind) {
            continue;
        }
        unsigned int slot = alloc_spill_slot(ctx);
        size_t offset = ctx->current_func_locals_size + slot * 4;
        fprintf(ctx->output, "    str %s, [sp, #%zu]\n", victim->name, offset);
        entry->kind = CODEGEN_STACK_ENTRY_SPILLED;
        entry->content.spill_slot = slot;
        victim->busy = 1;
        return victim;
    }

    cclynx_fatal_error("ERROR: too many registers\n");
}

static struct codegen_reg * pop_reg(struct codegen_context * ctx)
{
    assert(ctx != NULL);
    if (ctx->reg_stack_pos <= 0) {
        cclynx_fatal_error("ERROR: reg stack underflow for target arm64 generator\n");
    }
    struct codegen_stack_entry * entry = &ctx->reg_stack[ctx->reg_stack_pos - 1];
    if (entry->kind == CODEGEN_STACK_ENTRY_REAL) {
        struct codegen_reg * reg = entry->content.reg;
        entry->content.reg = NULL;
        --ctx->reg_stack_pos;
        return reg;
    }
    unsigned int slot = entry->content.spill_slot;
    --ctx->reg_stack_pos;
    struct codegen_reg * reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
    size_t offset = ctx->current_func_locals_size + slot * 4;
    fprintf(ctx->output, "    ldr %s, [sp, #%zu]\n", reg->name, offset);
    free_spill_slot(ctx, slot);
    return reg;
}

static void free_reg(struct codegen_reg * reg)
{
    assert(reg != NULL);

    reg->busy = 0;
}

static void simulator_alloc(struct simulator_context * ctx)
{
    assert(ctx != NULL);
    if (ctx->live_regs < CODEGEN_REG_COUNT) {
        ctx->live_regs++;
        return;
    }
    for (unsigned int k = 0; k < ctx->stack_pos; ++k) {
        if (!ctx->stack_is_spill[k]) {
            ctx->stack_is_spill[k] = 1;
            ctx->stack_spilled_count++;
            if (ctx->stack_spilled_count > ctx->max_spilled_live) {
                ctx->max_spilled_live = ctx->stack_spilled_count;
            }
            return;
        }
    }
    cclynx_fatal_error("ERROR: simulator cannot allocate register\n");
}

static void simulator_free(struct simulator_context * ctx)
{
    assert(ctx != NULL);
    assert(ctx->live_regs > 0);
    ctx->live_regs--;
}

static void simulator_push(struct simulator_context * ctx)
{
    assert(ctx != NULL);
    if (ctx->stack_pos >= CODEGEN_REG_STACK_SIZE) {
        cclynx_fatal_error("ERROR: simulator reg stack overflow\n");
    }
    ctx->stack_is_spill[ctx->stack_pos++] = 0;
}

static void simulator_pop(struct simulator_context * ctx)
{
    assert(ctx != NULL);
    assert(ctx->stack_pos > 0);
    ctx->stack_pos--;
    if (ctx->stack_is_spill[ctx->stack_pos]) {
        ctx->stack_spilled_count--;
        simulator_alloc(ctx);
    }
}

static unsigned int simulator_compute_max_spills(struct ir_function * func)
{
    assert(func != NULL);
    struct simulator_context ctx;
    memset(&ctx, 0, sizeof(ctx));

    for (size_t i = 0; i < func->instruction_count; ++i) {
        struct ir_instruction * ins = func->instructions[i];
        switch (ins->code) {
            case OP_FUNC:
            case OP_FUNC_END:
                break;
            case OP_LABEL:
            case OP_JUMP:
            case OP_NOP:
            case OP_STORE_PARAM:
                break;
            case OP_CONST:
            case OP_LOAD:
                simulator_alloc(&ctx);
                simulator_push(&ctx);
                break;
            case OP_STORE:
            case OP_JUMP_IF_FALSE:
            case OP_ARG:
                simulator_pop(&ctx);
                simulator_free(&ctx);
                break;
            case OP_JUMP_IF_EQ:
            case OP_JUMP_IF_NE:
            case OP_JUMP_IF_LTE:
            case OP_JUMP_IF_GTE:
                simulator_pop(&ctx);
                simulator_pop(&ctx);
                simulator_free(&ctx);
                simulator_free(&ctx);
                break;
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_LT:
            case OP_GT:
            case OP_EQ:
            case OP_NE:
                simulator_pop(&ctx);
                simulator_pop(&ctx);
                simulator_alloc(&ctx);
                simulator_free(&ctx);
                simulator_free(&ctx);
                simulator_push(&ctx);
                break;
            case OP_MOD:
                simulator_pop(&ctx);
                simulator_pop(&ctx);
                simulator_alloc(&ctx);
                simulator_alloc(&ctx);
                simulator_free(&ctx);
                simulator_free(&ctx);
                simulator_free(&ctx);
                simulator_push(&ctx);
                break;
            case OP_NEG:
            case OP_LOGICAL_NOT:
                simulator_pop(&ctx);
                simulator_alloc(&ctx);
                simulator_free(&ctx);
                simulator_push(&ctx);
                break;
            case OP_RETURN:
                if (ins->op1 != NULL) {
                    simulator_pop(&ctx);
                    simulator_free(&ctx);
                }
                break;
            case OP_CALL:
                simulator_alloc(&ctx);
                simulator_push(&ctx);
                break;
            default:
                break;
        }
    }
    return ctx.max_spilled_live;
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
    assert(program->function_count > 0);

    ctx->output = file;

    fprintf(file, ".text\n");
    fprintf(file, ".align 2\n");
    fprintf(file, "\n");

    for (size_t func_idx = 0; func_idx < program->function_count; ++func_idx) {
        struct ir_function * func = program->functions[func_idx];

    for (size_t i = 0; i < func->instruction_count; ++i) {
        struct ir_instruction * instruction = func->instructions[i];

        switch (instruction->code) {
            case OP_LABEL:
                fprintf(file, ".L%llu:\n", instruction->op1->content.label_id);
                break;
            case OP_JUMP:
                fprintf(file, "    b .L%llu\n", instruction->op1->content.label_id);
                break;
            case OP_FUNC:
                {
                    ctx->current_func_max_spills = simulator_compute_max_spills(func);
                    ctx->current_func_locals_size = instruction->result->content.function.local_vars_size;
                    memset(ctx->spill_slot_used, 0, sizeof(ctx->spill_slot_used));
                    size_t frame_size = align_up(ctx->current_func_locals_size + ctx->current_func_max_spills * 4, 16);
                    fprintf(file, ".global _%s\n", instruction->result->content.function.identifier->name);
                    fprintf(file, "_%s:\n", instruction->result->content.function.identifier->name);
                    fprintf(file, "    stp x29, x30, [sp, -16]!\n");
                    fprintf(file, "    mov x29, sp\n");
                    if (frame_size > 0) {
                        fprintf(file, "    sub sp, sp, #%zu\n", frame_size);
                    }
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
                    fprintf(file, "    cbz %s, .L%llu\n", op1_reg->name, instruction->op2->content.label_id);
                    free_reg(op1_reg);
                }
                break;
            case OP_JUMP_IF_EQ:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    b.eq .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_NE:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    b.ne .L%llu\n", instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_LTE:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    const char * cond = type_is_unsigned(instruction->op1->type) ? "b.ls" : "b.le";
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    %s .L%llu\n", cond, instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_JUMP_IF_GTE:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    const char * cond = type_is_unsigned(instruction->op1->type) ? "b.hs" : "b.ge";
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    %s .L%llu\n", cond, instruction->result->content.label_id);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                }
                break;
            case OP_GT:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    const char * cond = type_is_unsigned(instruction->op1->type) ? "hi" : "gt";
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    cset %s, %s\n", result_reg->name, cond);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_LT:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    const char * cond = type_is_unsigned(instruction->op1->type) ? "lo" : "lt";
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    cset %s, %s\n", result_reg->name, cond);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_EQ:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    cset %s, eq\n", result_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_NE:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    cmp %s, %s\n", op1_reg->name, op2_reg->name);
                    fprintf(file, "    cset %s, ne\n", result_reg->name);
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
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    mul %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_DIV:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    const char * op = type_is_unsigned(instruction->result->type) ? "udiv" : "sdiv";
                    fprintf(file, "    %s %s, %s, %s\n", op, result_reg->name, op1_reg->name, op2_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_MOD:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * quotient_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    const char * div_op = type_is_unsigned(instruction->result->type) ? "udiv" : "sdiv";
                    fprintf(file, "    %s %s, %s, %s\n", div_op, quotient_reg->name, op1_reg->name, op2_reg->name);
                    fprintf(file, "    msub %s, %s, %s, %s\n", result_reg->name, quotient_reg->name, op2_reg->name, op1_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    free_reg(quotient_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_SUB:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    sub %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_NEG:
                {
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    neg %s, %s\n", result_reg->name, op1_reg->name);
                    free_reg(op1_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_LOGICAL_NOT:
                {
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    cmp %s, #0\n", op1_reg->name);
                    fprintf(file, "    cset %s, eq\n", result_reg->name);
                    free_reg(op1_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_ADD:
                {
                    struct codegen_reg * op2_reg = pop_reg(ctx);
                    struct codegen_reg * op1_reg = pop_reg(ctx);
                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    add %s, %s, %s\n", result_reg->name, op1_reg->name, op2_reg->name);
                    free_reg(op1_reg);
                    free_reg(op2_reg);
                    push_reg(ctx, result_reg);
                }
                break;
            case OP_RETURN:
                {
                    if (instruction->op1 != NULL) {
                        struct codegen_reg * result_reg = pop_reg(ctx);
                        fprintf(file, "    mov w0, %s\n", result_reg->name);
                        free_reg(result_reg);
                    }

                    size_t locals = instruction->result->content.function.local_vars_size;
                    size_t frame_size = align_up(locals + ctx->current_func_max_spills * 4, 16);
                    if (frame_size > 0) {
                        fprintf(file, "    add sp, sp, #%zu\n", frame_size);
                    }
                    fprintf(file, "    ldp x29, x30, [sp], #16\n");
                    fprintf(file, "    ret\n");
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
                    size_t saved_real_count = 0;
                    for (size_t j = 0; j < ctx->reg_stack_pos; ++j) {
                        if (ctx->reg_stack[j].kind == CODEGEN_STACK_ENTRY_REAL) {
                            saved_real_count++;
                        }
                    }
                    size_t spill_memory_size = align_up(saved_real_count * 4, 16);

                    if (saved_real_count > 0) {
                        fprintf(file, "    sub sp, sp, #%zu\n", spill_memory_size);
                        size_t k = 0;
                        for (size_t j = 0; j < ctx->reg_stack_pos; ++j) {
                            if (ctx->reg_stack[j].kind != CODEGEN_STACK_ENTRY_REAL) {
                                continue;
                            }
                            fprintf(file, "    str %s, [sp, #%zu]\n", ctx->reg_stack[j].content.reg->name, k * 4);
                            k++;
                        }
                    }

                    fprintf(file, "    bl _%s\n", instruction->op1->content.function.identifier->name);

                    if (saved_real_count > 0) {
                        size_t k = 0;
                        for (size_t j = 0; j < ctx->reg_stack_pos; ++j) {
                            if (ctx->reg_stack[j].kind != CODEGEN_STACK_ENTRY_REAL) {
                                continue;
                            }
                            fprintf(file, "    ldr %s, [sp, #%zu]\n", ctx->reg_stack[j].content.reg->name, k * 4);
                            k++;
                        }
                        fprintf(file, "    add sp, sp, #%zu\n", spill_memory_size);
                    }

                    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
                    fprintf(file, "    mov %s, w0\n", result_reg->name);
                    push_reg(ctx, result_reg);
                }
                break;
            default:
                cclynx_fatal_error("ERROR: unknown instruction\n");
        }
    }

    }

    fflush(file);
}

void op_const(struct codegen_context * ctx, FILE * output, struct ir_operand * op1)
{
    assert(ctx != NULL);
    assert(output != NULL);
    assert(op1 != NULL);
    assert(op1->type != NULL);

    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);

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
}

void op_load(struct codegen_context * ctx, FILE * output, struct ir_operand * op1)
{
    assert(ctx != NULL);
    assert(output != NULL);
    assert(op1 != NULL);
    assert(op1->type != NULL);

    struct codegen_reg * result_reg = alloc_reg(ctx, CODEGEN_REG_KIND_INTEGER);
    fprintf(output, "    ldr %s, [sp, #%zu]\n", result_reg->name, op1->content.variable.offset);
    push_reg(ctx, result_reg);
}
