#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ir.h"
#include "allocator.h"
#include "parser.h"
#include "type.h"

static unsigned long long int temp_id = 0;

static void do_generate_ir(struct ir_program * program, const struct ast_node * node);
static void ir_emit(struct ir_program * program, struct ir_instruction * instruction);


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

    struct identifier * function_name = ast->content.function_definition.name;

    do_generate_ir(program, ast->content.function_definition.body);

    if (program->position > 0) {
        program->instructions[0]->label = function_name;
    }
}


void do_generate_ir(struct ir_program * program, const struct ast_node * node)
{
    assert(program != NULL);
    assert(node != NULL);

    switch(node->kind) {
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
            {
                main_pool_alloc(struct ir_instruction, instruction)

                switch (node->content.binary_expression.operation) {
                    case BINARY_OPERATION_MULTIPLY:
                        {
                            instruction->code = OP_MUL;

                            do_generate_ir(program, node->content.binary_expression.lhs);
                            instruction->op1 = program->instructions[program->position - 1]->result;

                            do_generate_ir(program, node->content.binary_expression.rhs);
                            instruction->op2 = program->instructions[program->position - 1]->result;
                        }
                        break;
                    case BINARY_OPERATION_DIVIDE:
                        {
                            instruction->code = OP_DIV;

                            do_generate_ir(program, node->content.binary_expression.lhs);
                            instruction->op1 = program->instructions[program->position - 1]->result;

                            do_generate_ir(program, node->content.binary_expression.rhs);
                            instruction->op2 = program->instructions[program->position - 1]->result;
                        }
                        break;
                    default:
                        fprintf(stderr, "ERROR: unknown operation\n");
                        exit(1);
                }

                main_pool_alloc(struct ir_operand, result)
                result->content.temp_id = ++temp_id;
                result->kind = OPERAND_KIND_TEMPORARY;

                instruction->result = result;

                ir_emit(program, instruction);
            }
            break;
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
            {
                main_pool_alloc(struct ir_instruction, instruction)

                switch (node->content.binary_expression.operation) {
                    case BINARY_OPERATION_ADDITION:
                        {
                            instruction->code = OP_ADD;

                            do_generate_ir(program, node->content.binary_expression.lhs);
                            instruction->op1 = program->instructions[program->position - 1]->result;

                            do_generate_ir(program, node->content.binary_expression.rhs);
                            instruction->op2 = program->instructions[program->position - 1]->result;
                        }
                        break;
                    case BINARY_OPERATION_SUBTRACTION:
                        {
                            instruction->code = OP_SUB;

                            do_generate_ir(program, node->content.binary_expression.lhs);
                            instruction->op1 = program->instructions[program->position - 1]->result;

                            do_generate_ir(program, node->content.binary_expression.rhs);
                            instruction->op2 = program->instructions[program->position - 1]->result;
                        }
                        break;
                    default:
                        fprintf(stderr, "ERROR: unknown operation\n");
                        exit(1);
                }

                main_pool_alloc(struct ir_operand, result)
                result->content.temp_id = ++temp_id;
                result->kind = OPERAND_KIND_TEMPORARY;

                instruction->result = result;

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

                main_pool_alloc(struct ir_operand, result)
                result->content.temp_id = ++temp_id;
                result->kind = OPERAND_KIND_TEMPORARY;

                instruction->result = result;

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
