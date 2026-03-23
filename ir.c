#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "allocator.h"
#include "parser.h"
#include "type.h"
#include "symbol.h"
#include "errors.h"

static struct ir_operand * alloc_operand(struct ir_context * ctx);
static struct ir_operand * find_variable_operand_by_symbol(struct ir_context * ctx, struct symbol * symbol);
static void do_generate_ir(struct ir_context * ctx, struct ir_program * program, const struct ast_node * node);
static void ir_emit(struct ir_program * program, struct ir_instruction * instruction);
static struct ir_operand * new_temporary_operand(struct ir_context * ctx);
static void ir_generate_condition(struct ir_context * ctx, struct ir_program * program, struct ast_node * condition, struct ir_operand * jump_label);

void ir_context_init(struct ir_context * ctx, struct memory_blob_pool * pool)
{
    assert(ctx != NULL);
    assert(pool != NULL);
    memset(ctx, 0, sizeof(struct ir_context));
    ctx->pool = pool;
}

void ir_program_init(struct ir_program * program, struct memory_blob_pool * pool)
{
    assert(program != NULL);
    assert(pool != NULL);

    program->capacity = INITIAL_INSTRUCTION_COUNT;
    program->position = 0;
    program->instructions = memory_blob_pool_alloc(pool, program->capacity * sizeof(struct ir_instruction *));
    memset(program->instructions, 0, program->capacity * sizeof(struct ir_instruction *));
}

void ir_program_generate(struct ir_context * ctx, struct ir_program * program, const struct ast_node * ast)
{
    assert(ctx != NULL);
    assert(program != NULL);
    assert(ast != NULL);

    if (ast->kind != AST_NODE_KIND_FUNCTION_DEFINITION) {
        cclynx_fatal_error("ERROR: expected function to generate it to IR\n");
    }

    {
        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_FUNC;

        struct ir_operand * result = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
        memset(result, 0, sizeof(struct ir_operand));
        result->kind = OPERAND_KIND_FUNCTION_NAME;
        result->content.function.identifier = ast->content.function_definition.name;

        instruction->result = result;

        ctx->current_func = instruction;

        ir_emit(program, instruction);
    }

    do_generate_ir(ctx, program, ast->content.function_definition.body);

    {
        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_FUNC_END;
        instruction->result = ctx->current_func->result;

        ir_emit(program, instruction);
    }
}


void do_generate_ir(struct ir_context * ctx, struct ir_program * program, const struct ast_node * node)
{
    assert(ctx != NULL);
    assert(program != NULL);
    assert(node != NULL);

    switch(node->kind) {
        case AST_NODE_KIND_IF_STATEMENT:
            {
                struct ir_operand * end_of_condition_label = NULL;

                struct ir_operand * end_of_if_label = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
                memset(end_of_if_label, 0, sizeof(struct ir_operand));
                end_of_if_label->content.label_id = ++ctx->label_id;
                end_of_if_label->type = &type_void;
                end_of_if_label->kind = OPERAND_KIND_LABEL;

                ir_generate_condition(ctx, program, node->content.if_statement.condition, end_of_if_label);

                if (
                    node->content.if_statement.true_branch->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                    && node->content.if_statement.true_branch->content.node == NULL
                ) {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_NOP;

                    ir_emit(program, instruction);
                } else {
                    do_generate_ir(ctx, program, node->content.if_statement.true_branch);
                }

                if (node->content.if_statement.false_branch != NULL) {
                    {
                        struct ir_operand * end_label = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
                        memset(end_label, 0, sizeof(struct ir_operand));
                        end_label->kind = OPERAND_KIND_LABEL;
                        end_label->type = &type_void;
                        end_label->content.label_id = ++ctx->label_id;
                        end_of_condition_label = end_label;
                    }
                    {
                        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                        memset(instruction, 0, sizeof(struct ir_instruction));
                        instruction->code = OP_JUMP;
                        instruction->op1 = end_of_condition_label;

                        ir_emit(program, instruction);
                    }
                    {
                        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                        memset(instruction, 0, sizeof(struct ir_instruction));
                        instruction->code = OP_LABEL;
                        instruction->op1 = end_of_if_label;

                        ir_emit(program, instruction);
                    }

                    if (
                        node->content.if_statement.false_branch->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                        && node->content.if_statement.false_branch->content.node == NULL
                    ) {
                        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                        memset(instruction, 0, sizeof(struct ir_instruction));
                        instruction->code = OP_NOP;

                        ir_emit(program, instruction);
                    } else {
                        do_generate_ir(ctx, program, node->content.if_statement.false_branch);
                    }
                }

                if (end_of_condition_label != NULL) {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_LABEL;
                    instruction->op1 = end_of_condition_label;

                    ir_emit(program, instruction);
                } else {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_LABEL;
                    instruction->op1 = end_of_if_label;

                    ir_emit(program, instruction);
                }

            }
            break;
        case AST_NODE_KIND_WHILE_STATEMENT:
            {
                struct ir_operand * start_of_loop_label = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
                memset(start_of_loop_label, 0, sizeof(struct ir_operand));
                start_of_loop_label->content.label_id = ++ctx->label_id;
                start_of_loop_label->type = &type_void;
                start_of_loop_label->kind = OPERAND_KIND_LABEL;

                {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_LABEL;
                    instruction->op1 = start_of_loop_label;

                    ir_emit(program, instruction);
                }

                struct ir_operand * end_of_loop_label = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
                memset(end_of_loop_label, 0, sizeof(struct ir_operand));
                end_of_loop_label->content.label_id = ++ctx->label_id;
                end_of_loop_label->type = &type_void;
                end_of_loop_label->kind = OPERAND_KIND_LABEL;

                ir_generate_condition(ctx, program, node->content.while_statement.condition, end_of_loop_label);

                if (
                    node->content.while_statement.body->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                    && node->content.while_statement.body->content.node == NULL
                ) {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_NOP;

                    ir_emit(program, instruction);
                } else {
                    do_generate_ir(ctx, program, node->content.while_statement.body);
                }

                {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_JUMP;
                    instruction->op1 = start_of_loop_label;

                    ir_emit(program, instruction);
                }

                {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_LABEL;
                    instruction->op1 = end_of_loop_label;

                    ir_emit(program, instruction);
                }
            }
            break;
        case AST_NODE_KIND_VARIABLE:
            {
                struct ir_operand * variable = find_variable_operand_by_symbol(ctx, node->content.symbol);

                if (variable == NULL) {
                    variable = alloc_operand(ctx);
                    variable->kind = OPERAND_KIND_VARIABLE;
                    variable->content.variable.symbol = node->content.symbol;
                    variable->content.variable.offset = ctx->current_func->result->content.function.local_vars_size;
                    variable->type = node->content.symbol->type;
                    ctx->current_func->result->content.function.local_vars_size += node->content.symbol->type->size;
                }

                ctx->last_variable = variable;

                if (ctx->is_assign) {
                    return;
                }

                struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                memset(instruction, 0, sizeof(struct ir_instruction));
                instruction->code = OP_LOAD;

                instruction->op1 = variable;

                instruction->result = new_temporary_operand(ctx);

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_EXPRESSION_STATEMENT:
            do_generate_ir(ctx, program, node->content.node);
            break;
        case AST_NODE_KIND_CAST_EXPRESSION:
            {
                enum opcode cast_opcode = OP_NOP;

                if (node->type->kind == TYPE_KIND_INTEGER) {
                    cast_opcode = OP_INT_CAST;
                } else if (node->type->kind == TYPE_KIND_FLOAT) {
                    cast_opcode = OP_FLOAT_CAST;
                } else {
                    cclynx_fatal_error("ERROR: unsupported cast to '%s' yet!\n", type_stringify(node->type));
                }

                do_generate_ir(ctx, program, node->content.node);

                struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                memset(instruction, 0, sizeof(struct ir_instruction));
                instruction->code = cast_opcode;
                instruction->op1 = program->instructions[program->position - 1]->result;
                instruction->result = new_temporary_operand(ctx);

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_ASSIGNMENT_EXPRESSION:
            {
                struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                memset(instruction, 0, sizeof(struct ir_instruction));

                switch (node->content.assignment.type) {
                    case ASSIGNMENT_REGULAR:
                        instruction->code = OP_STORE;
                        break;
                    default:
                        cclynx_fatal_error("ERROR: unknown assignment\n");
                }

                ctx->is_assign = 1;

                do_generate_ir(ctx, program, node->content.assignment.lhs);

                ctx->is_assign = 0;

                instruction->op1 = ctx->last_variable;

                do_generate_ir(ctx, program, node->content.assignment.initializer);
                instruction->op2 = program->instructions[program->position - 1]->result;

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            break;
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
        case AST_NODE_KIND_EQUALITY_EXPRESSION:
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
        case AST_NODE_KIND_RELATIONAL_EXPRESSION:
            {
                struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                memset(instruction, 0, sizeof(struct ir_instruction));

                switch (node->content.binary_expression.operation) {
                    case BINARY_OPERATION_MULTIPLY:
                        instruction->code = OP_MUL;
                        break;
                    case BINARY_OPERATION_DIVIDE:
                        instruction->code = OP_DIV;
                        break;
                    case BINARY_OPERATION_EQUALITY:
                        instruction->code = OP_IS_EQUAL;
                        break;
                    case BINARY_OPERATION_INEQUALITY:
                        instruction->code = OP_IS_NOT_EQUAL;
                        break;
                    case BINARY_OPERATION_LESS_THAN:
                        instruction->code = OP_IS_LESS_THAN;
                        break;
                    case BINARY_OPERATION_GREATER_THAN:
                        instruction->code = OP_IS_GREATER_THAN;
                        break;
                    case BINARY_OPERATION_ADDITION:
                        instruction->code = OP_ADD;
                        break;
                    case BINARY_OPERATION_SUBTRACTION:
                        instruction->code = OP_SUB;
                        break;
                    default:
                        cclynx_fatal_error("ERROR: unknown operation\n");
                }

                do_generate_ir(ctx, program, node->content.binary_expression.lhs);
                instruction->op1 = program->instructions[program->position - 1]->result;

                do_generate_ir(ctx, program, node->content.binary_expression.rhs);
                instruction->op2 = program->instructions[program->position - 1]->result;

                instruction->result = new_temporary_operand(ctx);

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_COMPOUND_STATEMENT:
            {
                struct ast_node_list * it = node->content.list;

                if (it == NULL) {
                    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                    memset(instruction, 0, sizeof(struct ir_instruction));
                    instruction->code = OP_NOP;

                    ir_emit(program, instruction);
                } else {
                    while (it != NULL) {
                        do_generate_ir(ctx, program, it->node);

                        it = it->next;
                    }
                }
            }
            break;
        case AST_NODE_KIND_RETURN_STATEMENT:
            {
                struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                memset(instruction, 0, sizeof(struct ir_instruction));
                instruction->code = OP_RETURN;
                instruction->result = ctx->current_func->result;

                if (node->content.node != NULL) {
                    do_generate_ir(ctx, program, node->content.node);
                    instruction->op1 = program->instructions[program->position - 1]->result;
                }

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
        case AST_NODE_KIND_FLOAT_CONSTANT:
            {
                struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
                memset(instruction, 0, sizeof(struct ir_instruction));
                instruction->code = OP_CONST;

                struct ir_operand * constant = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
                memset(constant, 0, sizeof(struct ir_operand));
                constant->type = node->type;

                if (constant->type->kind == TYPE_KIND_INTEGER) {
                    constant->content.int_value = node->content.constant.value.integer_constant;
                } else if (constant->type->kind == TYPE_KIND_FLOAT) {
                    constant->content.float_value = node->content.constant.value.float_constant;
                }

                constant->kind = OPERAND_KIND_CONSTANT;

                instruction->op1 = constant;
                instruction->result = new_temporary_operand(ctx);

                ir_emit(program, instruction);
            }
            break;
        default:
            cclynx_fatal_error("ERROR: unknown ast node for IR generator\n");
    }
}

void ir_emit(struct ir_program * program, struct ir_instruction * instruction)
{
    assert(program != NULL);
    assert(instruction != NULL);

    if (program->position == program->capacity) {
        cclynx_fatal_error("ERROR: too many instructions\n");
    }

    program->instructions[program->position++] = instruction;
}

struct ir_operand * new_temporary_operand(struct ir_context * ctx)
{
    assert(ctx != NULL);
    struct ir_operand * result = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
    memset(result, 0, sizeof(struct ir_operand));
    result->content.temp_id = ++ctx->temp_id;
    result->kind = OPERAND_KIND_TEMPORARY;
    return result;
}

struct ir_operand * alloc_operand(struct ir_context * ctx)
{
    assert(ctx != NULL);
    if (ctx->operand_pos >= IR_MAX_OPERAND_COUNT) {
        cclynx_fatal_error("ERROR: too many operands!\n");
    }
    struct ir_operand * operand = &ctx->operands[ctx->operand_pos++];
    memset(operand, 0, sizeof(struct ir_operand));
    return operand;
}

struct ir_operand * find_variable_operand_by_symbol(struct ir_context * ctx, struct symbol * symbol)
{
    assert(ctx != NULL);
    assert(symbol != NULL);

    for (size_t i = 0; i < ctx->operand_pos; ++i) {
        struct ir_operand * operand = &ctx->operands[i];

        if (operand->kind != OPERAND_KIND_VARIABLE)
            continue;

        if (operand->content.variable.symbol != symbol)
            continue;

        return operand;
    }

    return NULL;
}

void ir_generate_condition(struct ir_context * ctx, struct ir_program * program, struct ast_node * condition, struct ir_operand * jump_label)
{
    assert(ctx != NULL);
    assert(program != NULL);
    assert(condition != NULL);

    struct ir_operand * op1 = NULL, * op2 = NULL;

    if (
        condition->kind == AST_NODE_KIND_RELATIONAL_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN
    ) {
        do_generate_ir(ctx, program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(ctx, program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_JUMP_IF_GREATER_OR_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_RELATIONAL_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_GREATER_THAN
    ) {
        do_generate_ir(ctx, program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(ctx, program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_JUMP_IF_LESS_OR_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_EQUALITY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_EQUALITY
    ) {
        do_generate_ir(ctx, program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(ctx, program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_JUMP_IF_NOT_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_EQUALITY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_INEQUALITY
    ) {
        do_generate_ir(ctx, program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(ctx, program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_JUMP_IF_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else {
        do_generate_ir(ctx, program, condition);

        struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
        memset(instruction, 0, sizeof(struct ir_instruction));
        instruction->code = OP_JUMP_IF_FALSE;
        instruction->op1 = program->instructions[program->position - 1]->result;
        instruction->op2 = jump_label;

        ir_emit(program, instruction);
    }
}
