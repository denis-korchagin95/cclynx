#include <assert.h>
#include <stdio.h>

#include "print-ir.h"

#include "ir.h"
#include "symbol.h"
#include "identifier.h"
#include "error.h"

void print_ir_program(const struct ir_program * program, FILE * file)
{
    assert(program != NULL);
    assert(file != NULL);
    assert(program->function_count > 0);

    static char buf[1024] = {'\0'};

    for (size_t func_idx = 0; func_idx < program->function_count; ++func_idx) {
        struct ir_function * func = program->functions[func_idx];

    for (size_t idx = 0; idx < func->instruction_count; ++idx) {
        struct ir_instruction * instruction = func->instructions[idx];

        switch (instruction->code) {
            case OP_JUMP_IF_EQ:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "OP_JUMP_IF_EQ %s\n", buf);
                break;
            case OP_JUMP_IF_NE:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "OP_JUMP_IF_NE %s\n", buf);
                break;
            case OP_JUMP_IF_LT:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "OP_JUMP_IF_LT %s\n", buf);
                break;
            case OP_JUMP_IF_GT:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "OP_JUMP_IF_GT %s\n", buf);
                break;
            case OP_JUMP_IF_LTE:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "OP_JUMP_IF_LTE %s\n", buf);
                break;
            case OP_JUMP_IF_GTE:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "OP_JUMP_IF_GTE %s\n", buf);
                break;
            case OP_LABEL:
                snprintf(buf, sizeof(buf), "\".L%llu\"", instruction->op1->content.label_id);
                fprintf(file, "OP_LABEL %s\n", buf);
                break;
            case OP_JUMP:
                snprintf(buf, sizeof(buf), "\".L%llu\"", instruction->op1->content.label_id);
                fprintf(file, "OP_JUMP %s\n", buf);
                break;
            case OP_JUMP_IF_FALSE:
                snprintf(buf, sizeof(buf), "t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.label_id);
                fprintf(file, "OP_JUMP_IF_FALSE %s\n", buf);
                break;
            case OP_LT:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_LT %s\n", buf);
                break;
            case OP_GT:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_GT %s\n", buf);
                break;
            case OP_EQ:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_EQ %s\n", buf);
                break;
            case OP_NE:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_NE %s\n", buf);
                break;
            case OP_LTE:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_LTE %s\n", buf);
                break;
            case OP_GTE:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_GTE %s\n", buf);
                break;
            case OP_STORE:
                snprintf(buf, sizeof(buf), "t%llu", instruction->op2->content.temp_id);
                fprintf(file, "OP_STORE %s, %s\n", instruction->op1->content.variable.symbol->identifier->name, buf);
                break;
            case OP_LOAD:
                snprintf(buf, sizeof(buf), "t%llu", instruction->result->content.temp_id);
                fprintf(file, "OP_LOAD %s, %s\n", instruction->op1->content.variable.symbol->identifier->name, buf);
                break;
            case OP_FUNC:
                fprintf(file, "OP_FUNC \"%s\"\n", instruction->result->content.function.identifier->name);
                break;
            case OP_FUNC_END:
                fprintf(file, "OP_FUNC_END\n");
                break;
            case OP_SUB:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_SUB %s\n", buf);
                break;
            case OP_NEG:
                fprintf(file, "OP_NEG t%llu, t%llu\n", instruction->op1->content.temp_id, instruction->result->content.temp_id);
                break;
            case OP_LOGICAL_NOT:
                fprintf(file, "OP_LOGICAL_NOT t%llu, t%llu\n", instruction->op1->content.temp_id, instruction->result->content.temp_id);
                break;
            case OP_DIV:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_DIV %s\n", buf);
                break;
            case OP_MOD:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_MOD %s\n", buf);
                break;
            case OP_CONST:
                {
                    snprintf(buf, sizeof(buf), "%lld, t%llu", instruction->op1->content.int_value, instruction->result->content.temp_id);
                    fprintf(file, "OP_CONST %s\n", buf);
                }
                break;
            case OP_RETURN:
                if (instruction->op1 != NULL) {
                    snprintf(buf, sizeof(buf), "t%llu", instruction->op1->content.temp_id);
                    fprintf(file, "OP_RETURN %s\n", buf);
                } else {
                    fprintf(file, "OP_RETURN\n");
                }
                break;
            case OP_ADD:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_ADD %s\n", buf);
                break;
            case OP_MUL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "OP_MUL %s\n", buf);
                break;
            case OP_NOP:
                fprintf(file, "OP_NOP\n");
                break;
            case OP_CALL:
                fprintf(file, "OP_CALL \"%s\", t%llu\n", instruction->op1->content.function.identifier->name, instruction->result->content.temp_id);
                break;
            case OP_ARG:
                fprintf(file, "OP_ARG t%llu, %lld\n", instruction->op1->content.temp_id, instruction->op2->content.int_value);
                break;
            case OP_STORE_PARAM:
                fprintf(file, "OP_STORE_PARAM \"%s\", %lld\n", instruction->op1->content.variable.symbol->identifier->name, instruction->op2->content.int_value);
                break;
            default:
                cclynx_fatal_error("FATAL ERROR(print): Unknown instruction for IR program\n");
        }
    }

        if (func_idx + 1 < program->function_count) {
            fprintf(file, "\n");
        }
    }

    fflush(file);
}
