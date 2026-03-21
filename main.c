#include <stdio.h>
#include <stdlib.h>

#include "cclynx.h"
#include "errors.h"
#include "identifier.h"
#include "symbol.h"
#include "tokenizer.h"
#include "print.h"
#include "parser.h"
#include "ir.h"
#include "target-arm64.h"


enum output_stage {
    STAGE_ASM,
    STAGE_TOKENS,
    STAGE_AST,
    STAGE_AST_DOT,
    STAGE_IR,
};

const char * source_filename = NULL;
enum output_stage output_stage = STAGE_ASM;

static void parse_options(int argc, const char * argv[]);
static void show_usage(const char * program_name, FILE * output);


int main(const int argc, const char * argv[])
{
    parse_options(argc, argv);

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", sizeof("--") - 1) == 0) {
            continue;
        }

        source_filename = argv[i];
        break;
    }

    if (source_filename == NULL) {
        cclynx_fatal_error("No source given!\n");
    }

    FILE * source = fopen(source_filename, "r");

    if (source == NULL) {
        cclynx_fatal_error("Could not open file %s\n", source_filename);
    }

    struct cclynx_context ctx;
    cclynx_init(&ctx);
    init_keywords(&ctx.identifier_table, &ctx.pool);
    struct tokenizer_context tokenizer_ctx;
    tokenizer_init(&tokenizer_ctx, &ctx.identifier_table, &ctx.pool);
    init_symbols(&ctx.identifier_table, &ctx.pool);

    struct token * tokens = tokenizer_tokenize_file(&tokenizer_ctx, source);

    fclose(source);

    struct parser_context parser_ctx;
    parser_init_context(&parser_ctx, tokens, &ctx.pool, &ctx.global_scope);

    if (output_stage == STAGE_TOKENS) {
        struct token * it = tokens;
        while (it != &eos_token) {
            print_token(it, stdout);
            it = it->next;
        }
        goto cleanup;
    }

    struct ast_node * ast = parser_parse(&parser_ctx);

    if (output_stage == STAGE_AST) {
        print_ast(ast, stdout);
        goto cleanup;
    }

    if (output_stage == STAGE_AST_DOT) {
        print_ast_dot(ast, stdout);
        goto cleanup;
    }

    struct ir_context ir_ctx;
    ir_context_init(&ir_ctx, &ctx.pool);

    struct ir_program ir_program;
    ir_program_init(&ir_program, &ctx.pool);

    {
        struct ast_node_list * iterator = ast->content.list;
        while (iterator != NULL) {
            ir_program_generate(&ir_ctx, &ir_program, iterator->node);
            iterator = iterator->next;
        }
    }

    if (output_stage == STAGE_IR) {
        print_ir_program(&ir_program, stdout);
        goto cleanup;
    }

    struct codegen_context codegen_ctx;
    codegen_context_init(&codegen_ctx);
    target_arm64_generate(&codegen_ctx, &ir_program, stdout);

cleanup:
    cclynx_free(&ctx);

    return 0;
}


void parse_options(const int argc, const char * argv[])
{
    for(int i = 1; i < argc; ++i) {
        const char * arg = argv[i];

        if (strncmp(arg, "--emit-tokens", sizeof("--emit-tokens") - 1) == 0) {
            output_stage = STAGE_TOKENS;
            continue;
        }

        if (strncmp(arg, "--emit-ast-dot", sizeof("--emit-ast-dot") - 1) == 0) {
            output_stage = STAGE_AST_DOT;
            continue;
        }

        if (strncmp(arg, "--emit-ast", sizeof("--emit-ast") - 1) == 0) {
            output_stage = STAGE_AST;
            continue;
        }

        if (strncmp(arg, "--emit-ir", sizeof("--emit-ir") - 1) == 0) {
            output_stage = STAGE_IR;
            continue;
        }

        if (strncmp(arg, "--emit-asm", sizeof("--emit-asm") - 1) == 0) {
            output_stage = STAGE_ASM;
            continue;
        }

        if (strncmp(arg, "--help", sizeof("--help") - 1) == 0) {
            show_usage(argv[0], stdout);
            exit(0);
        }

        if (strncmp(arg, "--", sizeof("--") - 1) == 0) {
            cclynx_fatal_error("ERROR: unknown option \"%s\"\n", arg);
        }
    }
}

void show_usage(const char * program_name, FILE * output)
{
    fprintf(output, "Usage: %s [options] path\n\n\n", program_name);
    fprintf(output, "OPTIONS\n");
    fprintf(output, "\t--help\n\t    Show this message.\n\n");
    fprintf(output, "\t--emit-tokens\n\t    Produces tokens.\n\n");
    fprintf(output, "\t--emit-ast\n\t    Produces abstract syntax tree.\n\n");
    fprintf(output, "\t--emit-ast-dot\n\t    Produces abstract syntax tree in DOT format.\n\n");
    fprintf(output, "\t--emit-ir\n\t    Produces intermediate representation.\n\n");
    fprintf(output, "\t--emit-asm\n\t    Produces assembly (default).\n\n");
}
