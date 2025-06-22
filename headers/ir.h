#ifndef IR_H
#define IR_H 1

#include <stddef.h>

#define INITIAL_INSTRUCTION_COUNT (1024)

struct type;
struct ast_node;
struct identifier;

enum operand_kind
{
    OPERAND_KIND_CONSTANT = 1,
    OPERAND_KIND_TEMPORARY,
    OPERAND_KIND_FUNCTION_NAME,
    OPERAND_KIND_VARIABLE,
    OPERAND_KIND_LABEL,
};

struct ir_operand
{
    union
    {
        struct variable {
            struct symbol * symbol;
            size_t offset;
        } variable;
        struct function {
            struct identifier * identifier;
            size_t local_vars_size;
        } function;
        unsigned long long int temp_id;
        unsigned long long int label_id;
        long long int llic;
    } content;
    struct type * type;
    enum operand_kind kind;
};

enum opcode
{
    OP_NOP = 0,
    OP_CONST,
    OP_FUNC,
    OP_FUNC_END,
    OP_LOAD,
    OP_STORE,
    OP_RETURN,
    OP_IS_LESS_THAN,
    OP_IS_GREATER_THAN,
    OP_IS_EQUAL,
    OP_IS_NOT_EQUAL,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_BRANCH_LESS_THAN,
    OP_BRANCH_GREATER_THAN,
    OP_LABEL,
    OP_ADD,
    OP_MUL,
    OP_DIV,
    OP_SUB,
};

struct ir_instruction
{
    struct ir_operand * op1;
    struct ir_operand * op2;
    struct ir_operand * result;
    enum opcode code;
};

struct ir_program
{
    struct ir_instruction ** instructions;
    size_t position;
    size_t capacity;
};

void ir_program_init(struct ir_program * program);
void ir_program_generate(struct ir_program * program, const struct ast_node * ast);

#endif /* IR_H */
