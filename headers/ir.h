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
};

struct ir_operand
{
    union
    {
        unsigned long long int temp_id;
        long long int llic;
    } content;
    struct type * type;
    enum operand_kind kind;
};

enum opcode
{
    OP_CONST = 1,
    OP_RETURN,
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
    struct identifier * label;
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
