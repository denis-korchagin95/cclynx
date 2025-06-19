#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "target-arm64.h"
#include "ir.h"

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

    unsigned int regs[5] = {0};
    unsigned int reg_use_count = 0;
    const char * reg_names[] = {"x9", "x10", "x11", "x12", "x13"};

    unsigned int last_reg = 0;

    for (size_t i = 0; i < program->position; ++i) {
        struct ir_instruction * instruction = program->instructions[i];

        switch (instruction->code) {
            case OP_CONST:
                regs[reg_use_count++] = 1;
                sprintf(buf, "%lli", instruction->op1->content.llic);
                last_reg = reg_use_count - 1;
                fprintf(file, "%smov %s, #%s\n", "    ", reg_names[last_reg], buf);
                break;
            case OP_MUL:
                regs[reg_use_count++] = 1;
                last_reg = reg_use_count - 1;
                fprintf(file, "%smul %s, %s, %s\n", "    ", reg_names[last_reg], reg_names[reg_use_count - 3], reg_names[reg_use_count - 2]);
                break;
            case OP_ADD:
                regs[reg_use_count++] = 1;
                last_reg = reg_use_count - 1;
                fprintf(file, "%sadd %s, %s, %s\n", "    ", reg_names[last_reg], reg_names[reg_use_count - 3], reg_names[reg_use_count - 2]);
                break;
            case OP_RETURN:
                fprintf(file, "%smov x0, %s\n", "    ", reg_names[last_reg]);
                fprintf(file, "%sret\n", "    ");
                break;
            default:
                fprintf(stderr, "ERROR: unkown instruction\n");
                exit(1);
        }
    }

    fflush(file);
}
