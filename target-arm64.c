#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "target-arm64.h"
#include "ir.h"

static unsigned int regs[5] = {0};
static const char * reg_names[] = {"x9", "x10", "x11", "x12", "x13"};
static unsigned int stack[16] = {0};
static unsigned int stack_pos = 0;

static void push_reg(unsigned int reg)
{
	if (stack_pos >= 16) {
		fprintf(stderr, "ERROR: reg stack overflow for target arm64 generator\n");
		exit(1);
	}
	stack[stack_pos++] = reg;
}

static unsigned int pop_reg(void)
{
	if (stack_pos <= 0) {
		fprintf(stderr, "ERROR: reg stack underflow for target arm64 generator\n");
		exit(1);
	}
	unsigned int reg = stack[stack_pos - 1];
	stack[stack_pos - 1] = 0;
	--stack_pos;
	return reg;
}

static unsigned int alloc_reg(void)
{
    for (int i = 0; i < 5; ++i) {
        if (regs[i] == 0) {
            regs[i] = 1;
            return i + 1;
        }
    }

    fprintf(stderr, "ERROR: too many registers\n");
    exit(1);
}

static void free_reg(unsigned int reg)
{
	if (reg >= 1 && reg <= 5) {
    	regs[reg - 1] = 0;
		return;
	}

    fprintf(stderr, "ERROR: unknown register\n");
    exit(1);
}

static const char * get_reg_name(unsigned int reg)
{
	if (reg >= 1 && reg <= 5) {
		return reg_names[reg - 1];
	}

    fprintf(stderr, "ERROR: unknown register\n");
    exit(1);
}

void target_arm64_generate(struct ir_program * program, FILE * file)
{
    assert(program != NULL);
    assert(file != NULL);
    assert(program->position > 0);

    static char buf[1024] = {'\0'};

    fprintf(file, ".text\n");
    fprintf(file, ".globl _main\n");
    fprintf(file, ".align 2\n");
    fprintf(file, "\n");
    fprintf(file, "_main:\n");

    for (size_t i = 0; i < program->position; ++i) {
        struct ir_instruction * instruction = program->instructions[i];

        switch (instruction->code) {
            case OP_CONST:
				{
                	unsigned int op_reg = alloc_reg();
                	sprintf(buf, "%lli", instruction->op1->content.llic);
                	fprintf(file, "%smov %s, #%s\n", "    ", get_reg_name(op_reg), buf);
					push_reg(op_reg);
				}
                break;
            case OP_MUL:
				{
                	unsigned int op_reg = alloc_reg();
					unsigned int op2_reg = pop_reg();
					unsigned int op1_reg = pop_reg();
                	fprintf(file, "%smul %s, %s, %s\n", "    ", get_reg_name(op_reg), get_reg_name(op1_reg), get_reg_name(op2_reg));
					free_reg(op1_reg);
					free_reg(op2_reg);
					push_reg(op_reg);
				}
                break;
            case OP_ADD:
				{
                	unsigned int op_reg = alloc_reg();
					unsigned int op2_reg = pop_reg();
					unsigned int op1_reg = pop_reg();
                	fprintf(file, "%sadd %s, %s, %s\n", "    ", get_reg_name(op_reg), get_reg_name(op1_reg), get_reg_name(op2_reg));
					free_reg(op1_reg);
					free_reg(op2_reg);
					push_reg(op_reg);
				}
                break;
            case OP_RETURN:
				{
					unsigned int op_reg = pop_reg();
                	fprintf(file, "%smov x0, %s\n", "    ", get_reg_name(op_reg));
                	fprintf(file, "%sret\n", "    ");
                	free_reg(op_reg);
				}
                break;
            default:
                fprintf(stderr, "ERROR: unkown instruction\n");
                exit(1);
        }
    }

    fflush(file);
}
