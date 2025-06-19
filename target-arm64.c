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

    for (size_t i = 0; i < program->position; ++i) {
        struct ir_instruction * instruction = program->instructions[i];

        switch (instruction->code) {
            case OP_CONST:
                sprintf(buf, "%lli", instruction->op1->content.llic);
                fprintf(file, "%smov x9, #%s\n", "    ", buf);
                break;
            case OP_RETURN:
                fprintf(file, "%smov x0, x9\n", "    ");
                fprintf(file, "%sret\n", "    ");
                break;
            default:
                fprintf(stderr, "ERROR: unkown instruction\n");
                exit(1);
        }
    }

    fflush(file);
}
