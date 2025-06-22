#include <assert.h>
#include <stdlib.h>

#include "print.h"

#include "tokenizer.h"
#include "identifier.h"
#include "parser.h"
#include "symbol.h"
#include "ir.h"

static void do_print_ast(const struct ast_node * ast, FILE * file, int depth, unsigned int * ancestors_info, const char * node_label);

void print_token(const struct token * token, FILE * file)
{
    assert(token != NULL);
    assert(file != NULL);

    switch (token->kind) {
        case TOKEN_KIND_PUNCTUATOR:
            fprintf(file, "<TOKEN_PUNCTUATOR '%c'>\n", token->content.ch);
            break;
        case TOKEN_KIND_EOS:
            fprintf(file, "<TOKEN_EOS>\n");
            break;
        case TOKEN_KIND_UNKNOWN_CHARACTER:
            fprintf(file, "<TOKEN_UNKNOWN_CHARACTER '%c'>\n", token->content.ch);
            break;
        case TOKEN_KIND_IDENTIFIER:
            fprintf(file, "<TOKEN_%s '%s'>\n", token->content.identifier->is_keyword ? "KEYWORD" : "IDENTIFIER", token->content.identifier->name);
            break;
        case TOKEN_KIND_NUMBER:
            fprintf(file, "<TOKEN_NUMBER '%lld'>\n", token->content.integer_constant);
            break;
        case TOKEN_KIND_EQUAL_PUNCTUATOR:
            fprintf(file, "<TOKEN_SPECIAL_PUNCTUATOR '=='>\n");
            break;
        case TOKEN_KIND_NOT_EQUAL_PUNCTUATOR:
            fprintf(file, "<TOKEN_SPECIAL_PUNCTUATOR '!='>\n");
            break;
        default:
            fprintf(file, "<TOKEN_UNKNOWN>\n");
            exit(1);
    }

    fflush(file);
}


void print_ast(const struct ast_node * ast, FILE * file)
{
    assert(ast != NULL);
    assert(file != NULL);

    unsigned int ancestors_info[512] = {0};

    ancestors_info[0] = 1;

    do_print_ast(ast, file, 0, ancestors_info, NULL);
}

void do_print_ast(const struct ast_node * ast, FILE * file, int depth, unsigned int * ancestors_info, const char * node_label)
{
    assert(ast != NULL);
    assert(file != NULL);

    const unsigned int is_top = (ancestors_info[depth] & 1) > 0;

    if (is_top == 0) {
        for (int i = 0; i < depth - 1; ++i) {
            const unsigned int has_siblings = (ancestors_info[i] & 2) > 0;

            fprintf(file, "%s   ", has_siblings == 1 ? "|" : " ");
        }
        fprintf(file, "|\n");
        for (int i = 0; i < depth - 1; ++i) {
            const unsigned int has_siblings = (ancestors_info[i] & 2) > 0;

            fprintf(file, "%s   ", has_siblings == 1 ? "|" : " ");
        }
    }

    if (depth > 0) {
        fprintf(file, "+-> %s", node_label == NULL ? "" : node_label);
    }

    switch (ast->kind) {
        case AST_NODE_KIND_IF_STATEMENT:
            fprintf(file, "IfStatement\n");
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.if_statement.condition, file, depth + 1, ancestors_info, "(condition) ");
            ancestors_info[depth] = ast->content.if_statement.false_branch != NULL ? 2 : 0;
            do_print_ast(ast->content.if_statement.true_branch, file, depth + 1, ancestors_info, "(true branch) ");
            if (ast->content.if_statement.false_branch != NULL) {
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.if_statement.false_branch, file, depth + 1, ancestors_info, "(false branch) ");
            }
            break;
        case AST_NODE_KIND_RETURN_STATEMENT:
            fprintf(file, "ReturnStatement\n");
            ancestors_info[depth] = 0;
            if (ast->content.node != NULL)
                do_print_ast(ast->content.node, file, depth + 1, ancestors_info, NULL);
            break;
        case AST_NODE_KIND_WHILE_STATEMENT:
            fprintf(file, "WhileStatement\n");
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.while_statement.condition, file, depth + 1, ancestors_info, "(condition) ");
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.while_statement.body, file, depth + 1, ancestors_info, "(body) ");
            break;
        case AST_NODE_KIND_ASSIGNMENT_EXPRESSION:
            fprintf(file, "AssignmentExpression: '='\n");
            ancestors_info[depth] = ast->content.assignment.initializer != NULL ? 2 : 0;
            do_print_ast(ast->content.assignment.lhs, file, depth + 1, ancestors_info, NULL);
            if (ast->content.assignment.initializer != NULL) {
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.assignment.initializer, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_VARIABLE:
            fprintf(file, "Variable: {name: '%s', type: 'int'}\n", ast->content.variable->identifier->name);
            break;
        case AST_NODE_KIND_EXPRESSION_STATEMENT:
            fprintf(file, "ExpressionStatement%s\n", ast->content.node == NULL ? ": {empty expression}" : "");
            if (ast->content.node != NULL)
                do_print_ast(ast->content.node, file, depth + 1, ancestors_info, NULL);
            break;
        case AST_NODE_KIND_COMPOUND_STATEMENT:
            {
                fprintf(file, "CompoundStatement\n");
                struct ast_node_list * iterator = ast->content.list;
                while (iterator != NULL) {
                    ancestors_info[depth] = iterator->next == NULL ? 0 : 2;
                    do_print_ast(iterator->node, file, depth + 1, ancestors_info, NULL);
                    iterator = iterator->next;
                }
            }
            break;
        case AST_NODE_KIND_EQUALITY_EXPRESSION:
            fprintf(file, "EqualityExpression: '%s'\n", ast->content.binary_expression.operation == BINARY_OPERATION_EQUALITY ? "==" : "!=");
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            break;
        case AST_NODE_KIND_RELATIONAL_EXPRESSION:
            fprintf(file, "RelationalExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN ? '<' : '>');
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            break;
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
            fprintf(file, "AdditiveExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_ADDITION ? '+' : '-');
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            break;
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
            fprintf(file, "MultiplicativeExpression: '%c'\n", ast->content.binary_expression.operation == BINARY_OPERATION_MULTIPLY ? '*' : '/');
            ancestors_info[depth] = 2;
            do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
            ancestors_info[depth] = 0;
            do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
            fprintf(file, "IntegerConstant: '%llu'\n", ast->content.integer_constant);
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            fprintf(file, "VariableDeclaration: {name: '%s', type: 'int'}\n", ast->content.variable->identifier->name);
            break;
        case AST_NODE_KIND_FUNCTION_DEFINITION:
            fprintf(file, "FunctionDefinition: {name: '%s', type: 'int'}\n", ast->content.function_definition.name->name);
            if (ast->content.function_definition.body != NULL)
                do_print_ast(ast->content.function_definition.body, file, depth + 1, ancestors_info, NULL);
            break;
        default:
            fprintf(file, "<Unknown Ast Node>\n");
            exit(1);
    }

    fflush(file);
}

void print_ir_program(const struct ir_program * program, FILE * file)
{
    assert(program != NULL);
    assert(file != NULL);
    assert(program->position > 0);

    static char buf[1024] = {'\0'};

    for (size_t i = 0; i < program->position; ++i) {
        struct ir_instruction * instruction = program->instructions[i];

        switch (instruction->code) {
            case OP_BRANCH_LESS_THAN:
                sprintf(buf, "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "%04zu OP_BRANCH_LESS_THAN %s\n", i, buf);
                break;
            case OP_BRANCH_GREATER_THAN:
                sprintf(buf, "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "%04zu OP_BRANCH_GREATER_THAN %s\n", i, buf);
                break;
            case OP_LABEL:
                sprintf(buf, "\".L%llu\"", instruction->op1->content.label_id);
                fprintf(file, "%04zu OP_LABEL %s\n", i, buf);
                break;
            case OP_JUMP:
                sprintf(buf, "\".L%llu\"", instruction->op1->content.label_id);
                fprintf(file, "%04zu OP_JUMP %s\n", i, buf);
                break;
            case OP_JUMP_IF_FALSE:
                sprintf(buf, "t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.label_id);
                fprintf(file, "%04zu OP_JUMP_IF_FALSE %s\n", i, buf);
                break;
            case OP_IS_LESS_THAN:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_LESS_THAN %s\n", i, buf);
                break;
            case OP_IS_GREATER_THAN:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_GREATER_THAN %s\n", i, buf);
                break;
            case OP_IS_EQUAL:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_EQUAL %s\n", i, buf);
                break;
            case OP_IS_NOT_EQUAL:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_NOT_EQUAL %s\n", i, buf);
                break;
            case OP_STORE:
                sprintf(buf, "t%llu", instruction->op2->content.temp_id);
                fprintf(file, "%04zu OP_STORE %s, %s\n", i, instruction->op1->content.variable.symbol->identifier->name, buf);
                break;
            case OP_LOAD:
                sprintf(buf, "t%llu", instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_LOAD %s, %s\n", i, instruction->op1->content.variable.symbol->identifier->name, buf);
                break;
            case OP_FUNC:
                fprintf(file, "%04zu OP_FUNC \"%s\"\n", i, instruction->result->content.function.identifier->name);
                break;
            case OP_FUNC_END:
                fprintf(file, "%04zu OP_FUNC_END\n", i);
                break;
            case OP_SUB:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_SUB %s\n", i, buf);
            break;
            case OP_DIV:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_DIV %s\n", i, buf);
                break;
            case OP_CONST:
                sprintf(buf, "%lli, t%llu", instruction->op1->content.llic, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_CONST %s\n", i, buf);
            break;
            case OP_RETURN:
                sprintf(buf, "t%llu", instruction->op1->content.temp_id);
                fprintf(file, "%04zu OP_RETURN %s\n", i, buf);
            break;
            case OP_ADD:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_ADD %s\n", i, buf);
            break;
            case OP_MUL:
                sprintf(buf, "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_MUL %s\n", i, buf);
            break;
            default:
                fprintf(stderr, "ERROR: Unknown instruction for IR program\n");
                exit(1);
        }
    }

    fflush(file);
}
