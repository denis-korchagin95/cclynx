#ifndef CCLYNX_IR_H
#define CCLYNX_IR_H 1

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
        long long int int_value;
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
    OP_LT,
    OP_GT,
    OP_EQ,
    OP_NE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_LTE,
    OP_JUMP_IF_GTE,
    OP_JUMP_IF_NE,
    OP_JUMP_IF_EQ,
    OP_LABEL,
    OP_ADD,
    OP_MUL,
    OP_DIV,
    OP_UNSIGNED_DIV,
    OP_SUB,
    OP_CALL,
    OP_ARG,
    OP_STORE_PARAM,
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

#define IR_MAX_OPERAND_COUNT (1024)

struct memory_blob_pool;

struct ir_context
{
    struct memory_blob_pool * pool;
    unsigned long long int temp_id;
    unsigned long long int label_id;
    struct ir_operand * last_variable;
    unsigned int is_assign;
    struct ir_instruction * current_func;
    struct ir_operand operands[IR_MAX_OPERAND_COUNT];
    size_t operand_pos;
};

void ir_context_init(struct ir_context * ctx, struct memory_blob_pool * pool);
void ir_program_init(struct ir_program * program, struct memory_blob_pool * pool);
void ir_program_generate(struct ir_context * ctx, struct ir_program * program, const struct ast_node * ast);

#endif /* CCLYNX_IR_H */
