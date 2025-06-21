#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "allocator.h"
#include "parser.h"
#include "type.h"
#include "symbol.h"

#define MAX_OPERAND_COUNT (1024)

static unsigned long long int temp_id = 0;
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
        case AST_NODE_KIND_VARIABLE:
            {
                struct ir_operand * variable = find_variable_operand_by_symbol(node->content.variable);

                if (variable == NULL) {
                    variable = alloc_operand();
                    variable->kind = OPERAND_KIND_VARIABLE;
                    variable->content.variable.symbol = node->content.variable;
                    variable->content.variable.offset = current_func->result->content.function.local_vars_size;
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
            {
                main_pool_alloc(struct ir_instruction, instruction)

                switch (node->content.binary_expression.operation) {
                    case BINARY_OPERATION_MULTIPLY:
                        instruction->code = OP_MUL;
                        break;
                    case BINARY_OPERATION_DIVIDE:
                        instruction->code = OP_DIV;
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
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
            {
                main_pool_alloc(struct ir_instruction, instruction)

                switch (node->content.binary_expression.operation) {
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
            for (struct ast_node_list * it = node->content.list; it != NULL; it = it->next)
                do_generate_ir(program, it->node);
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
            {
                main_pool_alloc(struct ir_instruction, instruction)
                instruction->code = OP_CONST;

                main_pool_alloc(struct ir_operand, constant)
                constant->content.llic = node->content.integer_constant;
                constant->type = &type_integer;
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
