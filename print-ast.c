#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "print-ast.h"

#include "ast.h"
#include "symbol.h"
#include "identifier.h"
#include "type.h"
#include "error.h"

static void do_print_ast(const struct ast_node * ast, FILE * file, int depth, unsigned int * ancestors_info, const char * node_label);

static void print_tree_indent(FILE * file, int depth, unsigned int * ancestors_info)
{
    assert(file != NULL);
    assert(ancestors_info != NULL);

    for (int i = 0; i < depth; ++i) {
        fprintf(file, "%s", (ancestors_info[i] & 2) > 0 ? "│   " : "    ");
    }
}

static void print_tree_connector(FILE * file, int depth, unsigned int * ancestors_info)
{
    assert(file != NULL);
    assert(ancestors_info != NULL);

    print_tree_indent(file, depth, ancestors_info);
    fprintf(file, "%s", (ancestors_info[depth] & 2) > 0 ? "├── " : "└── ");
}

static void print_type_child_labeled(FILE * file, int depth, unsigned int * ancestors_info, const struct type * type, int has_next_sibling, const char * label)
{
    assert(file != NULL);
    assert(ancestors_info != NULL);
    assert(type != NULL);
    assert(label != NULL);

    ancestors_info[depth] = has_next_sibling ? 2 : 0;
    print_tree_connector(file, depth, ancestors_info);
    fprintf(file, "%s\n", label);

    int type_depth = depth + 1;

    ancestors_info[type_depth] = 2;
    print_tree_connector(file, type_depth, ancestors_info);
    fprintf(file, "category: 'basic'\n");

    ancestors_info[type_depth] = 0;
    print_tree_connector(file, type_depth, ancestors_info);
    fprintf(file, "descriptor: '%s'\n", type_stringify(type));
}

static void print_type_child(FILE * file, int depth, unsigned int * ancestors_info, const struct type * type, int has_next_sibling)
{
    assert(file != NULL);
    assert(ancestors_info != NULL);
    assert(type != NULL);

    print_type_child_labeled(file, depth, ancestors_info, type, has_next_sibling, "Type");
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

    if (depth > 0 && is_top == 0) {
        const unsigned int has_siblings = (ancestors_info[depth - 1] & 2) > 0;
        print_tree_indent(file, depth - 1, ancestors_info);
        fprintf(file, "%s", has_siblings ? "├── " : "└── ");
    } else if (depth > 0) {
        print_tree_indent(file, depth - 1, ancestors_info);
        fprintf(file, "└── ");
    }

    if (depth > 0 && node_label != NULL) {
        fprintf(file, "%s", node_label);
    }

    switch (ast->kind) {
        case AST_NODE_KIND_TRANSLATION_UNIT:
            {
                const char * filename = ast->content.translation_unit.filename;
                if (filename != NULL) {
                    const char * base = strrchr(filename, '/');
                    base = base != NULL ? base + 1 : filename;
                    fprintf(file, "TranslationUnit: '%s'\n", base);
                } else {
                    fprintf(file, "TranslationUnit\n");
                }
                struct ast_node_list * iterator = ast->content.translation_unit.list;
                while (iterator != NULL) {
                    ancestors_info[depth] = iterator->next == NULL ? 0 : 2;
                    do_print_ast(iterator->node, file, depth + 1, ancestors_info, NULL);
                    iterator = iterator->next;
                }
            }
            break;
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
        case AST_NODE_KIND_JUMP_STATEMENT:
            {
                fprintf(file, "JumpStatement\n");
                const char * op_str = "";
                switch (ast->content.jump_statement.operation) {
                    case JUMP_OPERATION_BREAK:  op_str = "break";  break;
                    case JUMP_OPERATION_CONTINUE: op_str = "continue"; break;
                    case JUMP_OPERATION_RETURN: op_str = "return"; break;
                    default:
                        cclynx_fatal_error("FATAL ERROR: unknown jump operation\n");
                }
                bool has_expression =
                    ast->content.jump_statement.operation == JUMP_OPERATION_RETURN
                    && ast->content.jump_statement.expression != NULL;
                ancestors_info[depth] = has_expression ? 2 : 0;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Operation: '%s'\n", op_str);
                if (has_expression) {
                    ancestors_info[depth] = 0;
                    do_print_ast(ast->content.jump_statement.expression, file, depth + 1, ancestors_info, NULL);
                }
            }
            break;
        case AST_NODE_KIND_ITERATION_STATEMENT:
            {
                const char * op_str = "";
                switch (ast->content.iteration_statement.operation) {
                    case ITERATION_OPERATION_WHILE: op_str = "while"; break;
                    default:
                        cclynx_fatal_error("FATAL ERROR: unknown iteration operation\n");
                }
                fprintf(file, "IterationStatement\n");
                ancestors_info[depth] = 2;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Operation: '%s'\n", op_str);
                do_print_ast(ast->content.iteration_statement.condition, file, depth + 1, ancestors_info, "(condition) ");
                ancestors_info[depth] = 0;
                do_print_ast(ast->content.iteration_statement.body, file, depth + 1, ancestors_info, "(body) ");
            }
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
        case AST_NODE_KIND_VARIABLE_EXPRESSION:
            fprintf(file, "VariableExpression: '%s'\n", ast->content.symbol->identifier->name);
            print_type_child(file, depth, ancestors_info, ast->type, 0);
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
        case AST_NODE_KIND_UNARY_EXPRESSION:
            {
                const char * unary_op_str = "";
                switch (ast->content.unary_expression.operation) {
                    case UNARY_OPERATION_NEGATE:      unary_op_str = "-"; break;
                    case UNARY_OPERATION_LOGICAL_NOT: unary_op_str = "!"; break;
                }
                fprintf(file, "UnaryExpression\n");
                ancestors_info[depth] = 2;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Operation: '%s'\n", unary_op_str);
                print_type_child(file, depth, ancestors_info, ast->type, 1);
                ancestors_info[depth] = 0;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Operand\n");
                ancestors_info[depth + 1] = 0;
                do_print_ast(ast->content.unary_expression.operand, file, depth + 2, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_BINARY_EXPRESSION:
            {
                const char * op_str = "";
                switch (ast->content.binary_expression.operation) {
                    case BINARY_OPERATION_EQUALITY:                 op_str = "=="; break;
                    case BINARY_OPERATION_INEQUALITY:               op_str = "!="; break;
                    case BINARY_OPERATION_LESS_THAN:                op_str = "<";  break;
                    case BINARY_OPERATION_GREATER_THAN:             op_str = ">";  break;
                    case BINARY_OPERATION_LESS_THAN_OR_EQUAL:       op_str = "<="; break;
                    case BINARY_OPERATION_GREATER_THAN_OR_EQUAL:    op_str = ">="; break;
                    case BINARY_OPERATION_ADDITION:                 op_str = "+";  break;
                    case BINARY_OPERATION_SUBTRACTION:              op_str = "-";  break;
                    case BINARY_OPERATION_MULTIPLY:                 op_str = "*";  break;
                    case BINARY_OPERATION_DIVIDE:                   op_str = "/";  break;
                    case BINARY_OPERATION_MODULO:                   op_str = "%";  break;
                    default:
                        cclynx_fatal_error("FATAL ERROR: unknown binary operation\n");
                }
                fprintf(file, "BinaryExpression\n");
                ancestors_info[depth] = 2;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Operation: '%s'\n", op_str);
                print_type_child(file, depth, ancestors_info, ast->type, 1);
                ancestors_info[depth] = 2;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Lhs\n");
                ancestors_info[depth + 1] = 0;
                do_print_ast(ast->content.binary_expression.lhs, file, depth + 2, ancestors_info, NULL);
                ancestors_info[depth] = 0;
                print_tree_connector(file, depth, ancestors_info);
                fprintf(file, "Rhs\n");
                ancestors_info[depth + 1] = 0;
                do_print_ast(ast->content.binary_expression.rhs, file, depth + 2, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_INTEGER_CONSTANT_EXPRESSION:
            {
                fprintf(file, "IntegerConstant: '%lld'\n", ast->content.constant.value);
                print_type_child(file, depth, ancestors_info, ast->type, 0);
            }
            break;
        case AST_NODE_KIND_VARIABLE_DECLARATION:
            {
                fprintf(file, "VariableDeclaration: '%s'\n", ast->content.symbol->identifier->name);
                print_type_child(file, depth, ancestors_info, ast->type, 0);
            }
            break;
        case AST_NODE_KIND_FUNCTION_PARAMETER:
            fprintf(file, "Parameter: '%s'\n", ast->content.symbol->identifier->name);
            print_type_child(file, depth, ancestors_info, ast->type, 0);
            break;
        case AST_NODE_KIND_FUNCTION_DEFINITION:
            {
                char args[32] = "";
                if (ast->content.function_definition.parameter_presence == PARAMETER_PRESENCE_VOID) {
                    snprintf(args, sizeof(args), " <no-parameters>");
                } else if (ast->content.function_definition.parameter_presence == PARAMETER_PRESENCE_UNSPECIFIED) {
                    snprintf(args, sizeof(args), " <unspecified-parameters>");
                } else if (ast->content.function_definition.parameter_presence == PARAMETER_PRESENCE_SPECIFIED) {
                    const char * suffix = ast->content.function_definition.parameter_count == 1 ? "parameter" : "parameters";
                    snprintf(args, sizeof(args), " <%d-%s>", ast->content.function_definition.parameter_count, suffix);
                }
                fprintf(file, "FunctionDefinition: '%s'%s\n", ast->content.function_definition.name->name, args);
                int has_params_or_body = ast->content.function_definition.parameter_count > 0 || ast->content.function_definition.body != NULL;
                print_type_child_labeled(file, depth, ancestors_info, ast->type, has_params_or_body, "ReturnType");
                if (ast->content.function_definition.parameter_count > 0) {
                    ancestors_info[depth] = ast->content.function_definition.body != NULL ? 2 : 0;

                    print_tree_indent(file, depth, ancestors_info);
                    fprintf(file, "%s", (ancestors_info[depth] & 2) > 0 ? "├── " : "└── ");
                    fprintf(file, "ParameterList\n");

                    for (unsigned int i = 0; i < ast->content.function_definition.parameter_count; i++) {
                        ancestors_info[depth + 1] = i < ast->content.function_definition.parameter_count - 1 ? 2 : 0;
                        do_print_ast(ast->content.function_definition.parameters[i], file, depth + 2, ancestors_info, NULL);
                    }
                }
                ancestors_info[depth] = 0;
                if (ast->content.function_definition.body != NULL)
                    do_print_ast(ast->content.function_definition.body, file, depth + 1, ancestors_info, NULL);
            }
            break;
        case AST_NODE_KIND_FUNCTION_CALL_EXPRESSION:
            {
                if (ast->content.function_call.argument_count == 0) {
                    fprintf(file, "FunctionCallExpression: '%s' <no-arguments>\n",
                        ast->content.function_call.function->identifier->name);
                } else {
                    fprintf(file, "FunctionCallExpression: '%s' <%d-argument%s>\n",
                        ast->content.function_call.function->identifier->name,
                        ast->content.function_call.argument_count,
                        ast->content.function_call.argument_count == 1 ? "" : "s");
                }
                print_type_child_labeled(file, depth, ancestors_info, ast->type, ast->content.function_call.argument_count > 0, "ReturnType");
                for (unsigned int i = 0; i < ast->content.function_call.argument_count; i++) {
                    ancestors_info[depth] = i < ast->content.function_call.argument_count - 1 ? 2 : 0;
                    do_print_ast(ast->content.function_call.arguments[i], file, depth + 1, ancestors_info, NULL);
                }
            }
            break;
        case AST_NODE_KIND_CAST_EXPRESSION:
            {
                fprintf(file, "CastExpression\n");
                print_type_child(file, depth, ancestors_info, ast->type, ast->content.node != NULL);
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
