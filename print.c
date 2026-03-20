#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "print.h"

#include "tokenizer.h"
#include "identifier.h"
#include "parser.h"
#include "symbol.h"
#include "ir.h"
#include "type.h"
#include "errors.h"

static void do_print_ast(const struct ast_node * ast, FILE * file, int depth, unsigned int * ancestors_info, const char * node_label);
static int do_print_ast_dot(const struct ast_node * ast, FILE * file, int next_id);

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
            fprintf(file, "<TOKEN_NUMBER '%s'>\n", token->content.number);
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
            fprintf(file, "Variable: '%s' {type: '%s'}\n", ast->content.variable->identifier->name, type_stringify(ast->type));
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
            {
                fprintf(file, "EqualityExpression: '%s' {type: '%s'}\n", ast->content.binary_expression.operation == BINARY_OPERATION_EQUALITY ? "==" : "!=", type_stringify(ast->type));
                ancestors_info[depth] = 2;
                do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_RELATIONAL_EXPRESSION:
            {
                fprintf(file, "RelationalExpression: '%c' {type: '%s'}\n", ast->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN ? '<' : '>', type_stringify(ast->type));
                ancestors_info[depth] = 2;
                do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
            {
                fprintf(file, "AdditiveExpression: '%c' {type: '%s'}\n", ast->content.binary_expression.operation == BINARY_OPERATION_ADDITION ? '+' : '-', type_stringify(ast->type));
                ancestors_info[depth] = 2;
                do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
            {
                fprintf(file, "MultiplicativeExpression: '%c' {type: '%s'}\n", ast->content.binary_expression.operation == BINARY_OPERATION_MULTIPLY ? '*' : '/', type_stringify(ast->type));
                ancestors_info[depth] = 2;
                do_print_ast(ast->content.binary_expression.lhs, file, depth + 1, ancestors_info, NULL);
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.binary_expression.rhs, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
            {
                fprintf(file, "IntegerConstant: '%lld' {type: '%s'}\n", ast->content.constant.value.integer_constant, type_stringify(ast->type));
            }
            break;
        case AST_NODE_KIND_FLOAT_CONSTANT:
            {
                fprintf(file, "FloatConstant: '%f' {type: '%s'}\n", ast->content.constant.value.float_constant, type_stringify(ast->type));
            }
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            {
                fprintf(file, "VariableDeclaration: '%s' {type: '%s'}\n", ast->content.variable->identifier->name, type_stringify(ast->type));
            }
            break;
        case AST_NODE_KIND_FUNCTION_DEFINITION:
            {
                fprintf(file, "FunctionDefinition: '%s' {type: '%s'}\n", ast->content.function_definition.name->name, type_stringify(ast->type));
                if (ast->content.function_definition.body != NULL)
                    do_print_ast(ast->content.function_definition.body, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_CAST_EXPRESSION:
            {
                fprintf(file, "CastExpression {type: '%s'}\n", type_stringify(ast->type));
                ancestors_info[depth] = 0;
                if (ast->content.node != NULL)
                    do_print_ast(ast->content.node, file, depth + 1, ancestors_info, NULL);
            }
            break;
        default:
            fprintf(file, "<Unknown Ast Node>\n");
            exit(1);
    }

    fflush(file);
}

void print_ast_dot(const struct ast_node * ast, FILE * file)
{
    assert(ast != NULL);
    assert(file != NULL);

    fprintf(file, "digraph AST {\n");
    fprintf(file, "    node [shape=box, fontname=\"monospace\"];\n");
    do_print_ast_dot(ast, file, 0);
    fprintf(file, "}\n");

    fflush(file);
}

int do_print_ast_dot(const struct ast_node * ast, FILE * file, int next_id)
{
    int id = next_id++;

    switch (ast->kind) {
        case AST_NODE_KIND_FUNCTION_DEFINITION:
            fprintf(file, "    n%d [label=\"FunctionDefinition\\n'%s' : %s\"];\n", id, ast->content.function_definition.name->name, type_stringify(ast->type));
            if (ast->content.function_definition.body != NULL) {
                int child_id = next_id;
                next_id = do_print_ast_dot(ast->content.function_definition.body, file, next_id);
                fprintf(file, "    n%d -> n%d;\n", id, child_id);
            }
            break;
        case AST_NODE_KIND_COMPOUND_STATEMENT:
            fprintf(file, "    n%d [label=\"CompoundStatement\"];\n", id);
            for (struct ast_node_list * it = ast->content.list; it != NULL; it = it->next) {
                int child_id = next_id;
                next_id = do_print_ast_dot(it->node, file, next_id);
                fprintf(file, "    n%d -> n%d;\n", id, child_id);
            }
            break;
        case AST_NODE_KIND_IF_STATEMENT:
            fprintf(file, "    n%d [label=\"IfStatement\"];\n", id);
            {
                int cond_id = next_id;
                next_id = do_print_ast_dot(ast->content.if_statement.condition, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"condition\"];\n", id, cond_id);
                int true_id = next_id;
                next_id = do_print_ast_dot(ast->content.if_statement.true_branch, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"true\"];\n", id, true_id);
                if (ast->content.if_statement.false_branch != NULL) {
                    int false_id = next_id;
                    next_id = do_print_ast_dot(ast->content.if_statement.false_branch, file, next_id);
                    fprintf(file, "    n%d -> n%d [label=\"false\"];\n", id, false_id);
                }
            }
            break;
        case AST_NODE_KIND_WHILE_STATEMENT:
            fprintf(file, "    n%d [label=\"WhileStatement\"];\n", id);
            {
                int cond_id = next_id;
                next_id = do_print_ast_dot(ast->content.while_statement.condition, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"condition\"];\n", id, cond_id);
                int body_id = next_id;
                next_id = do_print_ast_dot(ast->content.while_statement.body, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"body\"];\n", id, body_id);
            }
            break;
        case AST_NODE_KIND_RETURN_STATEMENT:
            fprintf(file, "    n%d [label=\"ReturnStatement\"];\n", id);
            if (ast->content.node != NULL) {
                int child_id = next_id;
                next_id = do_print_ast_dot(ast->content.node, file, next_id);
                fprintf(file, "    n%d -> n%d;\n", id, child_id);
            }
            break;
        case AST_NODE_KIND_EXPRESSION_STATEMENT:
            fprintf(file, "    n%d [label=\"ExpressionStatement\"];\n", id);
            if (ast->content.node != NULL) {
                int child_id = next_id;
                next_id = do_print_ast_dot(ast->content.node, file, next_id);
                fprintf(file, "    n%d -> n%d;\n", id, child_id);
            }
            break;
        case AST_NODE_KIND_ASSIGNMENT_EXPRESSION:
            fprintf(file, "    n%d [label=\"AssignmentExpression '='\"];\n", id);
            {
                int lhs_id = next_id;
                next_id = do_print_ast_dot(ast->content.assignment.lhs, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"lhs\"];\n", id, lhs_id);
                if (ast->content.assignment.initializer != NULL) {
                    int rhs_id = next_id;
                    next_id = do_print_ast_dot(ast->content.assignment.initializer, file, next_id);
                    fprintf(file, "    n%d -> n%d [label=\"rhs\"];\n", id, rhs_id);
                }
            }
            break;
        case AST_NODE_KIND_EQUALITY_EXPRESSION:
        case AST_NODE_KIND_RELATIONAL_EXPRESSION:
        case AST_NODE_KIND_ADDITIVE_EXPRESSION:
        case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
            {
                const char * kind_name = "";
                char op[3] = {0};
                switch (ast->kind) {
                    case AST_NODE_KIND_EQUALITY_EXPRESSION:
                        kind_name = "EqualityExpression";
                        snprintf(op, sizeof(op), "%s", ast->content.binary_expression.operation == BINARY_OPERATION_EQUALITY ? "==" : "!=");
                        break;
                    case AST_NODE_KIND_RELATIONAL_EXPRESSION:
                        kind_name = "RelationalExpression";
                        op[0] = ast->content.binary_expression.operation == BINARY_OPERATION_LESS_THAN ? '<' : '>';
                        break;
                    case AST_NODE_KIND_ADDITIVE_EXPRESSION:
                        kind_name = "AdditiveExpression";
                        op[0] = ast->content.binary_expression.operation == BINARY_OPERATION_ADDITION ? '+' : '-';
                        break;
                    case AST_NODE_KIND_MULTIPLICATIVE_EXPRESSION:
                        kind_name = "MultiplicativeExpression";
                        op[0] = ast->content.binary_expression.operation == BINARY_OPERATION_MULTIPLY ? '*' : '/';
                        break;
                    default:
                        break;
                }
                fprintf(file, "    n%d [label=\"%s '%s'\\n: %s\"];\n", id, kind_name, op, type_stringify(ast->type));
                int lhs_id = next_id;
                next_id = do_print_ast_dot(ast->content.binary_expression.lhs, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"lhs\"];\n", id, lhs_id);
                int rhs_id = next_id;
                next_id = do_print_ast_dot(ast->content.binary_expression.rhs, file, next_id);
                fprintf(file, "    n%d -> n%d [label=\"rhs\"];\n", id, rhs_id);
            }
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT:
            fprintf(file, "    n%d [label=\"IntegerConstant\\n%lld : %s\"];\n", id, ast->content.constant.value.integer_constant, type_stringify(ast->type));
            break;
        case AST_NODE_KIND_FLOAT_CONSTANT:
            fprintf(file, "    n%d [label=\"FloatConstant\\n%f : %s\"];\n", id, ast->content.constant.value.float_constant, type_stringify(ast->type));
            break;
        case AST_NODE_KIND_VARIABLE:
            fprintf(file, "    n%d [label=\"Variable\\n'%s' : %s\"];\n", id, ast->content.variable->identifier->name, type_stringify(ast->type));
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            fprintf(file, "    n%d [label=\"VariableDeclaration\\n'%s' : %s\"];\n", id, ast->content.variable->identifier->name, type_stringify(ast->type));
            break;
        case AST_NODE_KIND_CAST_EXPRESSION:
            fprintf(file, "    n%d [label=\"CastExpression\\n: %s\"];\n", id, type_stringify(ast->type));
            if (ast->content.node != NULL) {
                int child_id = next_id;
                next_id = do_print_ast_dot(ast->content.node, file, next_id);
                fprintf(file, "    n%d -> n%d;\n", id, child_id);
            }
            break;
        default:
            fprintf(file, "    n%d [label=\"Unknown\"];\n", id);
            break;
    }

    return next_id;
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
            case OP_FLOAT_CAST:
                snprintf(buf, sizeof(buf), "t%llu, t%llu", instruction->op1->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_FLOAT_CAST %s\n", i, buf);
                break;
            case OP_INT_CAST:
                snprintf(buf, sizeof(buf), "t%llu, t%llu", instruction->op1->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_INT_CAST %s\n", i, buf);
                break;
            case OP_JUMP_IF_EQUAL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "%04zu OP_JUMP_IF_EQUAL %s\n", i, buf);
                break;
            case OP_JUMP_IF_NOT_EQUAL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "%04zu OP_JUMP_IF_NOT_EQUAL %s\n", i, buf);
                break;
            case OP_JUMP_IF_LESS_OR_EQUAL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "%04zu OP_JUMP_IF_LESS_OR_EQUAL %s\n", i, buf);
                break;
            case OP_JUMP_IF_GREATER_OR_EQUAL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.label_id);
                fprintf(file, "%04zu OP_JUMP_IF_GREATER_OR_EQUAL %s\n", i, buf);
                break;
            case OP_LABEL:
                snprintf(buf, sizeof(buf), "\".L%llu\"", instruction->op1->content.label_id);
                fprintf(file, "%04zu OP_LABEL %s\n", i, buf);
                break;
            case OP_JUMP:
                snprintf(buf, sizeof(buf), "\".L%llu\"", instruction->op1->content.label_id);
                fprintf(file, "%04zu OP_JUMP %s\n", i, buf);
                break;
            case OP_JUMP_IF_FALSE:
                snprintf(buf, sizeof(buf), "t%llu, \".L%llu\"", instruction->op1->content.temp_id, instruction->op2->content.label_id);
                fprintf(file, "%04zu OP_JUMP_IF_FALSE %s\n", i, buf);
                break;
            case OP_IS_LESS_THAN:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_LESS_THAN %s\n", i, buf);
                break;
            case OP_IS_GREATER_THAN:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_GREATER_THAN %s\n", i, buf);
                break;
            case OP_IS_EQUAL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_EQUAL %s\n", i, buf);
                break;
            case OP_IS_NOT_EQUAL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_IS_NOT_EQUAL %s\n", i, buf);
                break;
            case OP_STORE:
                snprintf(buf, sizeof(buf), "t%llu", instruction->op2->content.temp_id);
                fprintf(file, "%04zu OP_STORE %s, %s\n", i, instruction->op1->content.variable.symbol->identifier->name, buf);
                break;
            case OP_LOAD:
                snprintf(buf, sizeof(buf), "t%llu", instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_LOAD %s, %s\n", i, instruction->op1->content.variable.symbol->identifier->name, buf);
                break;
            case OP_FUNC:
                fprintf(file, "%04zu OP_FUNC \"%s\"\n", i, instruction->result->content.function.identifier->name);
                break;
            case OP_FUNC_END:
                fprintf(file, "%04zu OP_FUNC_END\n", i);
                break;
            case OP_SUB:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_SUB %s\n", i, buf);
                break;
            case OP_DIV:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_DIV %s\n", i, buf);
                break;
            case OP_CONST:
                {
                    if (instruction->op1->type->kind == TYPE_KIND_INTEGER) {
                        snprintf(buf, sizeof(buf), "%lld, t%llu", instruction->op1->content.int_value, instruction->result->content.temp_id);
                    } else if (instruction->op1->type->kind == TYPE_KIND_FLOAT) {
                        snprintf(buf, sizeof(buf), "%f, t%llu", instruction->op1->content.float_value, instruction->result->content.temp_id);
                    } else {
                        snprintf(buf, sizeof(buf), "<unknown constant>");
                    }
                    fprintf(file, "%04zu OP_CONST %s\n", i, buf);
                }
                break;
            case OP_RETURN:
                if (instruction->op1 != NULL) {
                    snprintf(buf, sizeof(buf), "t%llu", instruction->op1->content.temp_id);
                    fprintf(file, "%04zu OP_RETURN %s\n", i, buf);
                } else {
                    fprintf(file, "%04zu OP_RETURN\n", i);
                }
                break;
            case OP_ADD:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_ADD %s\n", i, buf);
                break;
            case OP_MUL:
                snprintf(buf, sizeof(buf), "t%llu, t%llu, t%llu", instruction->op1->content.temp_id, instruction->op2->content.temp_id, instruction->result->content.temp_id);
                fprintf(file, "%04zu OP_MUL %s\n", i, buf);
                break;
            case OP_NOP:
                fprintf(file, "%04zu OP_NOP\n", i);
                break;
            default:
                cclynx_fatal_error("ERROR(print): Unknown instruction for IR program\n");
        }
    }

    fflush(file);
}
