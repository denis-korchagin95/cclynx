#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "allocator.h"
#include "parser.h"
#include "type.h"
#include "symbol.h"
#include "type.h"

#define MAX_OPERAND_COUNT (1024)

static unsigned long long int temp_id = 0;
static unsigned long long int label_id = 0;
static struct ir_operand * last_variable = NULL;
static unsigned int is_assign = 0;
static struct ir_instruction * current_func = NULL;

static struct ir_operand operands[MAX_OPERAND_COUNT] = {0};
static size_t operand_pos = 0;

static struct ir_operand * alloc_operand(void);
static struct ir_operand * find_variable_operand_by_symbol(struct symbol * symbol);
static void do_generate_ir(struct ir_program * program, const struct ast_node * node);
static void ir_emit(struct ir_program * program, struct ir_instruction * instruction);
static struct ir_operand * new_temporary_operand(void);
static void ir_generate_condition(struct ir_program * program, struct ast_node * condition, struct ir_operand * jump_label);

void ir_program_init(struct ir_program * program)
{
    assert(program != NULL);

    program->capacity = INITIAL_INSTRUCTION_COUNT;
    program->position = 0;
    program->instructions = (struct ir_instruction **) memory_blob_pool_alloc(&main_pool, program->capacity * sizeof(struct ir_instruction *));
    memset(program->instructions, 0, program->capacity * sizeof(struct ir_instruction *));
}

void ir_program_generate(struct ir_program * program, const struct ast_node * ast)
{
    assert(program != NULL);
    assert(ast != NULL);

    if (ast->kind != AST_NODE_KIND_FUNCTION_DEFINITION) {
        fprintf(stderr, "ERROR: expected function to generate it to IR\n");
        exit(1);
    }

    {
        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_FUNC;

        main_pool_alloc(struct ir_operand, result)
        result->kind = OPERAND_KIND_FUNCTION_NAME;
        result->content.function.identifier = ast->content.function_definition.name;

        instruction->result = result;

        current_func = instruction;

        ir_emit(program, instruction);
    }

    do_generate_ir(program, ast->content.function_definition.body);

    {
        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_FUNC_END;

        ir_emit(program, instruction);
    }
}


void do_generate_ir(struct ir_program * program, const struct ast_node * node)
{
    assert(program != NULL);
    assert(node != NULL);

    switch(node->kind) {
        case AST_NODE_KIND_IF_STATEMENT:
            {
                struct ir_operand * end_of_condition_label = NULL;

                main_pool_alloc(struct ir_operand, end_of_if_label)
                end_of_if_label->content.label_id = ++label_id;
                end_of_if_label->type = &type_void;
                end_of_if_label->kind = OPERAND_KIND_LABEL;

                ir_generate_condition(program, node->content.if_statement.condition, end_of_if_label);

                if (
                    node->content.if_statement.true_branch->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                    && node->content.if_statement.true_branch->content.node == NULL
                ) {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_NOP;

                    ir_emit(program, instruction);
                } else {
                    do_generate_ir(program, node->content.if_statement.true_branch);
                }

                if (node->content.if_statement.false_branch != NULL) {
                    {
                        main_pool_alloc(struct ir_operand, end_label)
                        end_label->kind = OPERAND_KIND_LABEL;
                        end_label->type = &type_void;
                        end_label->content.label_id = ++label_id;
                        end_of_condition_label = end_label;
                    }
                    {
                        main_pool_alloc(struct ir_instruction, instruction)
                        instruction->code = OP_JUMP;
                        instruction->op1 = end_of_condition_label;

                        ir_emit(program, instruction);
                    }
                    {
                        main_pool_alloc(struct ir_instruction, instruction)
                        instruction->code = OP_LABEL;
                        instruction->op1 = end_of_if_label;

                        ir_emit(program, instruction);
                    }

                    if (
                        node->content.if_statement.false_branch->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                        && node->content.if_statement.false_branch->content.node == NULL
                    ) {
                        main_pool_alloc(struct ir_instruction, instruction)
                        instruction->code = OP_NOP;

                        ir_emit(program, instruction);
                    } else {
                        do_generate_ir(program, node->content.if_statement.false_branch);
                    }
                }

                if (end_of_condition_label != NULL) {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_LABEL;
                    instruction->op1 = end_of_condition_label;

                    ir_emit(program, instruction);
                } else {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_LABEL;
                    instruction->op1 = end_of_if_label;

                    ir_emit(program, instruction);
                }

            }
            break;
        case AST_NODE_KIND_WHILE_STATEMENT:
            {
                main_pool_alloc(struct ir_operand, start_of_loop_label)
                start_of_loop_label->content.label_id = ++label_id;
                start_of_loop_label->type = &type_void;
                start_of_loop_label->kind = OPERAND_KIND_LABEL;

                {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_LABEL;
                    instruction->op1 = start_of_loop_label;

                    ir_emit(program, instruction);
                }

                main_pool_alloc(struct ir_operand, end_of_loop_label)
                end_of_loop_label->content.label_id = ++label_id;
                end_of_loop_label->type = &type_void;
                end_of_loop_label->kind = OPERAND_KIND_LABEL;

                ir_generate_condition(program, node->content.while_statement.condition, end_of_loop_label);

                if (
                    node->content.while_statement.body->kind == AST_NODE_KIND_EXPRESSION_STATEMENT
                    && node->content.while_statement.body->content.node == NULL
                ) {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_NOP;

                    ir_emit(program, instruction);
                } else {
                    do_generate_ir(program, node->content.while_statement.body);
                }

                {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_JUMP;
                    instruction->op1 = start_of_loop_label;

                    ir_emit(program, instruction);
                }

                {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_LABEL;
                    instruction->op1 = end_of_loop_label;

                    ir_emit(program, instruction);
                }
            }
            break;
        case AST_NODE_KIND_VARIABLE:
            {
                struct ir_operand * variable = find_variable_operand_by_symbol(node->content.variable);

                if (variable == NULL) {
                    variable = alloc_operand();
                    variable->kind = OPERAND_KIND_VARIABLE;
                    variable->content.variable.symbol = node->content.variable;
                    variable->content.variable.offset = current_func->result->content.function.local_vars_size;
                    variable->type = node->content.variable->type;
                    current_func->result->content.function.local_vars_size += node->content.variable->type->size;
                }

                last_variable = variable;

                if (is_assign) {
                    return;
                }

                main_pool_alloc(struct ir_instruction, instruction)
                instruction->code = OP_LOAD;

                instruction->op1 = variable;

                instruction->result = new_temporary_operand();

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_EXPRESSION_STATEMENT:
            do_generate_ir(program, node->content.node);
            break;
        case AST_NODE_KIND_CAST_EXPRESSION:
            {
                enum opcode cast_opcode = OP_NOP;

                if (node->type->kind == TYPE_KIND_INTEGER) {
                    cast_opcode = OP_INT_CAST;
                } else if (node->type->kind == TYPE_KIND_FLOAT) {
                    cast_opcode = OP_FLOAT_CAST;
                } else {
                    fprintf(stderr, "ERROR: unsupported cast to '%s' yet!\n", type_stringify(node->type));
                    exit(1);
                }

                do_generate_ir(program, node->content.node);

                main_pool_alloc(struct ir_instruction, instruction)
                instruction->code = cast_opcode;
                instruction->op1 = program->instructions[program->position - 1]->result;
                instruction->result = new_temporary_operand();

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_ASSIGNMENT_EXPRESSION:
            {
                main_pool_alloc(struct ir_instruction, instruction)

                switch (node->content.assignment.type) {
                    case ASSIGNMENT_REGULAR:
                        instruction->code = OP_STORE;
                        break;
                    default:
                        fprintf(stderr, "ERROR: unknown assignment\n");
                        exit(1);
                }

                is_assign = 1;

                do_generate_ir(program, node->content.assignment.lhs);

                is_assign = 0;

                instruction->op1 = last_variable;

                do_generate_ir(program, node->content.assignment.initializer);
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
                main_pool_alloc(struct ir_instruction, instruction)

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
                        fprintf(stderr, "ERROR: unknown operation\n");
                        exit(1);
                }

                do_generate_ir(program, node->content.binary_expression.lhs);
                instruction->op1 = program->instructions[program->position - 1]->result;

                do_generate_ir(program, node->content.binary_expression.rhs);
                instruction->op2 = program->instructions[program->position - 1]->result;

                instruction->result = new_temporary_operand();

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_COMPOUND_STATEMENT:
            {
                struct ast_node_list * it = node->content.list;

                if (it == NULL) {
                    main_pool_alloc(struct ir_instruction, instruction)
                    instruction->code = OP_NOP;

                    ir_emit(program, instruction);
                } else {
                    while (it != NULL) {
                        do_generate_ir(program, it->node);

                        it = it->next;
                    }
                }
            }
            break;
        case AST_NODE_KIND_RETURN_STATEMENT:
            {
                do_generate_ir(program, node->content.node);

                main_pool_alloc(struct ir_instruction, instruction)
                instruction->code = OP_RETURN;
                instruction->op1 = program->instructions[program->position - 1]->result;

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
        case AST_NODE_KIND_FLOAT_CONSTANT:
            {
                main_pool_alloc(struct ir_instruction, instruction)
                instruction->code = OP_CONST;

                main_pool_alloc(struct ir_operand, constant)
                constant->type = node->type;

                if (constant->type->kind == TYPE_KIND_INTEGER) {
                    constant->content.int_value = node->content.constant.value.integer_constant;
                } else if (constant->type->kind == TYPE_KIND_FLOAT) {
                    constant->content.float_value = node->content.constant.value.float_constant;
                }

                constant->kind = OPERAND_KIND_CONSTANT;

                instruction->op1 = constant;
                instruction->result = new_temporary_operand();

                ir_emit(program, instruction);
            }
            break;
        default:
            fprintf(stderr, "ERROR: unknown ast node for IR generator\n");
            exit(1);
    }
}

void ir_emit(struct ir_program * program, struct ir_instruction * instruction)
{
    assert(program != NULL);
    assert(instruction != NULL);

    if (program->position == program->capacity) {
        fprintf(stderr, "ERROR: too many instructions\n");
        exit(1);
    }

    program->instructions[program->position++] = instruction;
}

struct ir_operand * new_temporary_operand(void)
{
    main_pool_alloc(struct ir_operand, result)
    result->content.temp_id = ++temp_id;
    result->kind = OPERAND_KIND_TEMPORARY;
    return result;
}

struct ir_operand * alloc_operand(void)
{
    if (operand_pos >= MAX_OPERAND_COUNT) {
        fprintf(stderr, "ERROR: too many operands!\n");
        exit(1);
    }
    struct ir_operand * operand = &operands[operand_pos++];
    memset(operand, 0, sizeof(struct ir_operand));
    return operand;
}

struct ir_operand * find_variable_operand_by_symbol(struct symbol * symbol)
{
    assert(symbol != NULL);

    for (size_t i = 0; i < MAX_OPERAND_COUNT; ++i) {
        struct ir_operand * operand = &operands[i];

        if (operand->kind != OPERAND_KIND_VARIABLE)
            continue;

        if (operand->content.variable.symbol != symbol)
            continue;

        return operand;
    }

    return NULL;
}

void ir_generate_condition(struct ir_program * program, struct ast_node * condition, struct ir_operand * jump_label)
{
    assert(program != NULL);
    assert(condition != NULL);

    struct ir_operand * op1 = NULL, * op2 = NULL;

    if (
        condition->kind == AST_NODE_KIND_RELATIONAL_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN
    ) {
        do_generate_ir(program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_JUMP_IF_GREATER_OR_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_RELATIONAL_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_GREATER_THAN
    ) {
        do_generate_ir(program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_JUMP_IF_LESS_OR_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_EQUALITY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_EQUALITY
    ) {
        do_generate_ir(program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_JUMP_IF_NOT_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else if (
        condition->kind == AST_NODE_KIND_EQUALITY_EXPRESSION
        && condition->content.binary_expression.operation == BINARY_OPERATION_INEQUALITY
    ) {
        do_generate_ir(program, condition->content.binary_expression.lhs);
        op1 = program->instructions[program->position - 1]->result;
        do_generate_ir(program, condition->content.binary_expression.rhs);
        op2 = program->instructions[program->position - 1]->result;

        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_JUMP_IF_EQUAL;
        instruction->op1 = op1;
        instruction->op2 = op2;
        instruction->result = jump_label;

        ir_emit(program, instruction);
    } else {
        do_generate_ir(program, condition);

        main_pool_alloc(struct ir_instruction, instruction)
        instruction->code = OP_JUMP_IF_FALSE;
        instruction->op1 = program->instructions[program->position - 1]->result;
        instruction->op2 = jump_label;

        ir_emit(program, instruction);
    }
}
