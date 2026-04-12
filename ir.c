#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "allocator.h"
#include "ast.h"
#include "type.h"
#include "symbol.h"
#include "error.h"

static struct ir_instruction * ir_create_instruction(struct ir_context * ctx, enum opcode code);
static struct ir_operand * ir_create_operand(struct ir_context * ctx, enum operand_kind kind);
static struct ir_operand * alloc_operand(struct ir_context * ctx);
static void do_generate_ir(struct ir_context * ctx, struct ir_function * function, const struct ast_node * node);
static void ir_emit(struct ir_function * function, struct ir_instruction * instruction);
static struct ir_operand * new_temporary_operand(struct ir_context * ctx, struct ir_function * function);
static void ir_generate_condition(struct ir_context * ctx, struct ir_function * function, struct ast_node * condition, struct ir_operand * jump_label);

void ir_context_init(struct ir_context * ctx, struct memory_blob_pool * pool)
{
    assert(ctx != NULL);
    assert(pool != NULL);
    memset(ctx, 0, sizeof(struct ir_context));
    ctx->pool = pool;
}

void ir_program_init(struct ir_program * program)
{
    assert(program != NULL);

    program->function_capacity = 0;
    program->function_count = 0;
    program->functions = NULL;
}

void ir_program_free(struct ir_program * program)
{
    assert(program != NULL);

    for (size_t i = 0; i < program->function_count; i++) {
        free(program->functions[i]->instructions);
    }
    free(program->functions);
    program->functions = NULL;
    program->function_count = 0;
    program->function_capacity = 0;
}

static struct ir_function * ir_program_add_function(struct ir_program * program, struct memory_blob_pool * pool)
{
    assert(program != NULL);
    assert(pool != NULL);

    if (program->function_count == program->function_capacity) {
        size_t new_capacity = program->function_capacity == 0 ? IR_INITIAL_FUNCTION_CAPACITY : program->function_capacity * 2;
        struct ir_function ** new_functions = realloc(program->functions, new_capacity * sizeof(struct ir_function *));
        if (new_functions == NULL) {
            cclynx_fatal_error("ERROR: failed to allocate functions\n");
        }
        program->functions = new_functions;
        program->function_capacity = new_capacity;
    }

    struct ir_function * function = memory_blob_pool_alloc(pool, sizeof(struct ir_function));
    function->instructions = NULL;
    function->instruction_count = 0;
    function->instruction_capacity = 0;

    program->functions[program->function_count++] = function;
    return function;
}

void ir_program_generate(struct ir_context * ctx, struct ir_program * program, const struct ast_node * ast)
{
    assert(ctx != NULL);
    assert(program != NULL);
    assert(ast != NULL);

    if (ast->kind != AST_NODE_KIND_FUNCTION_DEFINITION) {
        cclynx_fatal_error("ERROR: expected function to generate it to IR\n");
    }

    struct ir_function * function = ir_program_add_function(program, ctx->pool);

    {
        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_FUNC);

        struct ir_operand * result = ir_create_operand(ctx, OPERAND_KIND_FUNCTION_NAME);
        result->content.function.identifier = ast->content.function_definition.name;

        instruction->result = result;

        ctx->current_func = instruction;

        ir_emit(function, instruction);
    }

    for (unsigned int i = 0; i < ast->content.function_definition.parameter_count; i++) {
        struct ast_node * param = ast->content.function_definition.parameters[i];
        struct symbol * param_symbol = param->content.symbol;

        struct ir_operand * variable = alloc_operand(ctx);
        variable->kind = OPERAND_KIND_VARIABLE;
        variable->content.variable.symbol = param_symbol;
        variable->content.variable.offset = ctx->current_func->result->content.function.local_vars_size;
        variable->type = param_symbol->type;
        param_symbol->ir_operand = variable;
        ctx->current_func->result->content.function.local_vars_size += param_symbol->type->size;

        struct ir_instruction * store_param = ir_create_instruction(ctx, OP_STORE_PARAM);
        store_param->op1 = variable;

        struct ir_operand * index = ir_create_operand(ctx, OPERAND_KIND_CONSTANT);
        index->content.int_value = i;
        store_param->op2 = index;

        ir_emit(function, store_param);
    }

    do_generate_ir(ctx, function, ast->content.function_definition.body);

    {
        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_FUNC_END);
        instruction->result = ctx->current_func->result;

        ir_emit(function, instruction);
    }
}


void do_generate_ir(struct ir_context * ctx, struct ir_function * function, const struct ast_node * node)
{
    assert(ctx != NULL);
    assert(function != NULL);
    assert(node != NULL);

    switch(node->kind) {
        case AST_NODE_KIND_IF_STATEMENT:
            {
                struct ir_operand * end_of_condition_label = NULL;

                struct ir_operand * end_of_if_label = ir_create_operand(ctx, OPERAND_KIND_LABEL);
                end_of_if_label->content.label_id = ++ctx->label_id;
                end_of_if_label->type = &type_void;

                ir_generate_condition(ctx, function, node->content.if_statement.condition, end_of_if_label);

                if (
                    node->content.if_statement.true_branch->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                    && node->content.if_statement.true_branch->content.node == NULL
                ) {
                    ir_emit(function, ir_create_instruction(ctx, OP_NOP));
                } else {
                    do_generate_ir(ctx, function, node->content.if_statement.true_branch);
                }

                if (node->content.if_statement.false_branch != NULL) {
                    {
                        struct ir_operand * end_label = ir_create_operand(ctx, OPERAND_KIND_LABEL);
                        end_label->type = &type_void;
                        end_label->content.label_id = ++ctx->label_id;
                        end_of_condition_label = end_label;
                    }
                    {
                        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP);
                        instruction->op1 = end_of_condition_label;

                        ir_emit(function, instruction);
                    }
                    {
                        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_LABEL);
                        instruction->op1 = end_of_if_label;

                        ir_emit(function, instruction);
                    }

                    if (
                        node->content.if_statement.false_branch->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                        && node->content.if_statement.false_branch->content.node == NULL
                    ) {
                        ir_emit(function, ir_create_instruction(ctx, OP_NOP));
                    } else {
                        do_generate_ir(ctx, function, node->content.if_statement.false_branch);
                    }
                }

                if (end_of_condition_label != NULL) {
                    struct ir_instruction * instruction = ir_create_instruction(ctx, OP_LABEL);
                    instruction->op1 = end_of_condition_label;

                    ir_emit(function, instruction);
                } else {
                    struct ir_instruction * instruction = ir_create_instruction(ctx, OP_LABEL);
                    instruction->op1 = end_of_if_label;

                    ir_emit(function, instruction);
                }

            }
            break;
        case AST_NODE_KIND_ITERATION_STATEMENT:
            {
                if (node->content.iteration_statement.operation != ITERATION_OPERATION_WHILE) {
                    cclynx_fatal_error("ERROR: unknown iteration operation\n");
                }

                struct ir_operand * start_of_loop_label = ir_create_operand(ctx, OPERAND_KIND_LABEL);
                start_of_loop_label->content.label_id = ++ctx->label_id;
                start_of_loop_label->type = &type_void;

                {
                    struct ir_instruction * instruction = ir_create_instruction(ctx, OP_LABEL);
                    instruction->op1 = start_of_loop_label;

                    ir_emit(function, instruction);
                }

                struct ir_operand * end_of_loop_label = ir_create_operand(ctx, OPERAND_KIND_LABEL);
                end_of_loop_label->content.label_id = ++ctx->label_id;
                end_of_loop_label->type = &type_void;

                ir_generate_condition(ctx, function, node->content.iteration_statement.condition, end_of_loop_label);

                struct ir_operand * previous_loop_start_label = ctx->current_loop_start_label;
                struct ir_operand * previous_loop_end_label = ctx->current_loop_end_label;
                ctx->current_loop_start_label = start_of_loop_label;
                ctx->current_loop_end_label = end_of_loop_label;

                if (
                    node->content.iteration_statement.body->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                    && node->content.iteration_statement.body->content.node == NULL
                ) {
                    ir_emit(function, ir_create_instruction(ctx, OP_NOP));
                } else {
                    do_generate_ir(ctx, function, node->content.iteration_statement.body);
                }

                ctx->current_loop_start_label = previous_loop_start_label;
                ctx->current_loop_end_label = previous_loop_end_label;

                {
                    struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP);
                    instruction->op1 = start_of_loop_label;

                    ir_emit(function, instruction);
                }

                {
                    struct ir_instruction * instruction = ir_create_instruction(ctx, OP_LABEL);
                    instruction->op1 = end_of_loop_label;

                    ir_emit(function, instruction);
                }
            }
            break;
        case AST_NODE_KIND_VARIABLE_EXPRESSION:
            {
                struct ir_operand * variable = node->content.symbol->ir_operand;

                if (variable == NULL) {
                    variable = alloc_operand(ctx);
                    variable->kind = OPERAND_KIND_VARIABLE;
                    variable->content.variable.symbol = node->content.symbol;
                    variable->content.variable.offset = ctx->current_func->result->content.function.local_vars_size;
                    variable->type = node->content.symbol->type;
                    node->content.symbol->ir_operand = variable;
                    ctx->current_func->result->content.function.local_vars_size += node->content.symbol->type->size;
                }

                ctx->last_variable = variable;

                if (ctx->is_assign) {
                    return;
                }

                struct ir_instruction * instruction = ir_create_instruction(ctx, OP_LOAD);

                instruction->op1 = variable;

                instruction->result = new_temporary_operand(ctx, function);
                instruction->result->type = node->type;

                ir_emit(function, instruction);
            }
            break;
        case AST_NODE_KIND_EXPRESSION_STATEMENT:
            do_generate_ir(ctx, function, node->content.node);
            break;
        case AST_NODE_KIND_CAST_EXPRESSION:
            {
                do_generate_ir(ctx, function, node->content.node);
                /* integer-to-integer casts (signedness change) are no-ops at IR level */
            }
            break;
        case AST_NODE_KIND_ASSIGNMENT_EXPRESSION:
            {
                struct ir_instruction * instruction = ir_create_instruction(ctx, OP_NOP);

                switch (node->content.assignment.type) {
                    case ASSIGNMENT_REGULAR:
                        instruction->code = OP_STORE;
                        break;
                    default:
                        cclynx_fatal_error("ERROR: unknown assignment\n");
                }

                ctx->is_assign = 1;

                do_generate_ir(ctx, function, node->content.assignment.lhs);

                ctx->is_assign = 0;

                instruction->op1 = ctx->last_variable;

                do_generate_ir(ctx, function, node->content.assignment.initializer);
                instruction->op2 = ir_last_instruction(function)->result;

                ir_emit(function, instruction);
            }
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            break;
        case AST_NODE_KIND_BINARY_EXPRESSION:
            {
                struct ir_instruction * instruction = ir_create_instruction(ctx, OP_NOP);

                switch (node->content.binary_expression.operation) {
                    case BINARY_OPERATION_MULTIPLY:
                        instruction->code = OP_MUL;
                        break;
                    case BINARY_OPERATION_DIVIDE:
                        instruction->code = OP_DIV;
                        break;
                    case BINARY_OPERATION_MODULO:
                        instruction->code = OP_MOD;
                        break;
                    case BINARY_OPERATION_EQUALITY:
                        instruction->code = OP_EQ;
                        break;
                    case BINARY_OPERATION_INEQUALITY:
                        instruction->code = OP_NE;
                        break;
                    case BINARY_OPERATION_LESS_THAN:
                        instruction->code = OP_LT;
                        break;
                    case BINARY_OPERATION_GREATER_THAN:
                        instruction->code = OP_GT;
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

                do_generate_ir(ctx, function, node->content.binary_expression.lhs);
                instruction->op1 = ir_last_instruction(function)->result;

                do_generate_ir(ctx, function, node->content.binary_expression.rhs);
                instruction->op2 = ir_last_instruction(function)->result;

                instruction->result = new_temporary_operand(ctx, function);
                instruction->result->type = node->type;

                ir_emit(function, instruction);
            }
            break;
        case AST_NODE_KIND_UNARY_EXPRESSION:
            {
                struct ir_instruction * instruction = ir_create_instruction(ctx, OP_NOP);

                switch (node->content.unary_expression.operation) {
                    case UNARY_OPERATION_NEGATE:
                        instruction->code = OP_NEG;
                        break;
                    case UNARY_OPERATION_LOGICAL_NOT:
                        instruction->code = OP_LOGICAL_NOT;
                        break;
                    default:
                        cclynx_fatal_error("ERROR: unknown unary operation\n");
                }

                do_generate_ir(ctx, function, node->content.unary_expression.operand);
                instruction->op1 = ir_last_instruction(function)->result;

                instruction->result = new_temporary_operand(ctx, function);
                instruction->result->type = node->type;

                ir_emit(function, instruction);
            }
            break;
        case AST_NODE_KIND_COMPOUND_STATEMENT:
            {
                struct ast_node_list * it = node->content.list;

                if (it == NULL) {
                    ir_emit(function, ir_create_instruction(ctx, OP_NOP));
                } else {
                    while (it != NULL) {
                        do_generate_ir(ctx, function, it->node);

                        it = it->next;
                    }
                }
            }
            break;
        case AST_NODE_KIND_JUMP_STATEMENT:
            switch (node->content.jump_statement.operation) {
                case JUMP_OPERATION_BREAK:
                    {
                        assert(ctx->current_loop_end_label != NULL);

                        struct ir_instruction * jump = ir_create_instruction(ctx, OP_JUMP);
                        jump->op1 = ctx->current_loop_end_label;

                        ir_emit(function, jump);
                    }
                    break;
                case JUMP_OPERATION_CONTINUE:
                    {
                        assert(ctx->current_loop_start_label != NULL);

                        struct ir_instruction * jump = ir_create_instruction(ctx, OP_JUMP);
                        jump->op1 = ctx->current_loop_start_label;

                        ir_emit(function, jump);
                    }
                    break;
                case JUMP_OPERATION_RETURN:
                    {
                        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_RETURN);
                        instruction->result = ctx->current_func->result;

                        if (node->content.jump_statement.expression != NULL) {
                            do_generate_ir(ctx, function, node->content.jump_statement.expression);
                            instruction->op1 = ir_last_instruction(function)->result;
                        }

                        ir_emit(function, instruction);
                    }
                    break;
                default:
                    cclynx_fatal_error("ERROR: unknown jump operation\n");
            }
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT_EXPRESSION:
            {
                struct ir_instruction * instruction = ir_create_instruction(ctx, OP_CONST);

                struct ir_operand * constant = ir_create_operand(ctx, OPERAND_KIND_CONSTANT);
                constant->type = node->type;
                constant->content.int_value = node->content.constant.value;

                instruction->op1 = constant;
                instruction->result = new_temporary_operand(ctx, function);
                instruction->result->type = node->type;

                ir_emit(function, instruction);
            }
            break;
        case AST_NODE_KIND_FUNCTION_CALL_EXPRESSION:
            {
                for (unsigned int i = 0; i < node->content.function_call.argument_count; i++) {
                    do_generate_ir(ctx, function, node->content.function_call.arguments[i]);

                    struct ir_instruction * arg_instruction = ir_create_instruction(ctx, OP_ARG);
                    arg_instruction->op1 = ir_last_instruction(function)->result;

                    struct ir_operand * index = ir_create_operand(ctx, OPERAND_KIND_CONSTANT);
                    index->content.int_value = i;
                    arg_instruction->op2 = index;

                    ir_emit(function, arg_instruction);
                }

                struct ir_instruction * call_instruction = ir_create_instruction(ctx, OP_CALL);

                struct ir_operand * callee = ir_create_operand(ctx, OPERAND_KIND_FUNCTION_NAME);
                callee->content.function.identifier = node->content.function_call.function->identifier;
                call_instruction->op1 = callee;

                call_instruction->result = new_temporary_operand(ctx, function);
                call_instruction->result->type = node->type;

                ir_emit(function, call_instruction);
            }
            break;
        default:
            cclynx_fatal_error("ERROR: unknown ast node for IR generator\n");
    }
}

void ir_emit(struct ir_function * function, struct ir_instruction * instruction)
{
    assert(function != NULL);
    assert(instruction != NULL);

    if (function->instruction_count == function->instruction_capacity) {
        size_t new_capacity = function->instruction_capacity == 0 ? IR_INITIAL_INSTRUCTION_CAPACITY : function->instruction_capacity * 2;
        struct ir_instruction ** new_instructions = realloc(function->instructions, new_capacity * sizeof(struct ir_instruction *));
        if (new_instructions == NULL) {
            cclynx_fatal_error("ERROR: failed to allocate instructions\n");
        }
        function->instructions = new_instructions;
        function->instruction_capacity = new_capacity;
    }

    function->instructions[function->instruction_count++] = instruction;
}

struct ir_instruction * ir_create_instruction(struct ir_context * ctx, enum opcode code)
{
    assert(ctx != NULL);
    struct ir_instruction * instruction = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_instruction));
    instruction->code = code;
    return instruction;
}

struct ir_operand * ir_create_operand(struct ir_context * ctx, enum operand_kind kind)
{
    assert(ctx != NULL);
    struct ir_operand * operand = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
    operand->kind = kind;
    return operand;
}

struct ir_operand * new_temporary_operand(struct ir_context * ctx, struct ir_function * function)
{
    assert(ctx != NULL);
    assert(function != NULL);
    struct ir_operand * result = ir_create_operand(ctx, OPERAND_KIND_TEMPORARY);
    result->content.temp_id = ++function->temp_id;
    return result;
}

struct ir_operand * alloc_operand(struct ir_context * ctx)
{
    assert(ctx != NULL);
    struct ir_operand * operand = memory_blob_pool_alloc(ctx->pool, sizeof(struct ir_operand));
    memset(operand, 0, sizeof(struct ir_operand));
    return operand;
}


void ir_generate_condition(struct ir_context * ctx, struct ir_function * function, struct ast_node * condition, struct ir_operand * jump_label)
{
    assert(ctx != NULL);
    assert(function != NULL);
    assert(condition != NULL);

    struct ir_operand * op1 = NULL, * op2 = NULL;

    if (
        condition->kind == AST_NODE_KIND_BINARY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN
    ) {
        do_generate_ir(ctx, function, condition->content.binary_expression.lhs);
        op1 = ir_last_instruction(function)->result;
        do_generate_ir(ctx, function, condition->content.binary_expression.rhs);
        op2 = ir_last_instruction(function)->result;

        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP_IF_GTE);
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(function, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_BINARY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_GREATER_THAN
    ) {
        do_generate_ir(ctx, function, condition->content.binary_expression.lhs);
        op1 = ir_last_instruction(function)->result;
        do_generate_ir(ctx, function, condition->content.binary_expression.rhs);
        op2 = ir_last_instruction(function)->result;

        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP_IF_LTE);
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(function, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_BINARY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_EQUALITY
    ) {
        do_generate_ir(ctx, function, condition->content.binary_expression.lhs);
        op1 = ir_last_instruction(function)->result;
        do_generate_ir(ctx, function, condition->content.binary_expression.rhs);
        op2 = ir_last_instruction(function)->result;

        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP_IF_NE);
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(function, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_BINARY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_INEQUALITY
    ) {
        do_generate_ir(ctx, function, condition->content.binary_expression.lhs);
        op1 = ir_last_instruction(function)->result;
        do_generate_ir(ctx, function, condition->content.binary_expression.rhs);
        op2 = ir_last_instruction(function)->result;

        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP_IF_EQ);
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(function, instruction);
    } else {
        do_generate_ir(ctx, function, condition);

        struct ir_instruction * instruction = ir_create_instruction(ctx, OP_JUMP_IF_FALSE);
        instruction->op1 = ir_last_instruction(function)->result;
        instruction->op2 = jump_label;

        ir_emit(function, instruction);
    }
}
