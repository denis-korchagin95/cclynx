#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cclynx.h"
#include "error.h"
#include "identifier.h"
#include "symbol.h"
#include "source.h"
#include "tokenizer.h"
#include "print.h"
#include "parser.h"
#include "warning.h"
#include "ir.h"
#include "target-arm64.h"


enum output_stage {
    STAGE_ASM,
    STAGE_TOKENS,
    STAGE_AST,
    STAGE_IR,
};

enum output_format {
    FORMAT_TREE,
    FORMAT_DOT,
};

const char * source_filename = NULL;
enum output_stage output_stage = STAGE_ASM;
enum output_format output_format = FORMAT_TREE;
bool output_format_explicit = false;
struct warning_flags warning_flags;

static void parse_options(int argc, const char * argv[]);
static void show_usage(const char * program_name, FILE * output);


int main(const int argc, const char * argv[])
{
    warning_init_default(&warning_flags);
    parse_options(argc, argv);

    if (output_format_explicit && output_stage != STAGE_AST) {
        cclynx_fatal_error("ERROR: --format is only supported with --emit-ast\n");
    }

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", sizeof("--") - 1) == 0 || strncmp(argv[i], "-W", sizeof("-W") - 1) == 0) {
            continue;
        }

        source_filename = argv[i];
        break;
    }

    if (source_filename == NULL) {
        cclynx_fatal_error("No source given!\n");
    }

    struct cclynx_context ctx;
    cclynx_init(&ctx);

    struct source source;
    source_load(&source, source_filename);

    init_keywords(&ctx.identifier_table, &ctx.pool);
    struct tokenizer_context tokenizer_ctx;
    tokenizer_init(&tokenizer_ctx, &ctx.identifier_table, &ctx.pool);
    init_symbols(&ctx.identifier_table, &ctx.pool);

    struct token * tokens = tokenizer_tokenize_file(&tokenizer_ctx, &source);

    struct parser_context parser_ctx;
    parser_init_context(&parser_ctx, tokens, &ctx.pool, &ctx.global_scope, source_filename);
    parser_ctx.warning_flags = warning_flags;

    if (output_stage == STAGE_TOKENS) {
        struct token * it = tokens;
        while (it != &eos_token) {
            print_token(it, stdout);
            it = it->next;
        }
        print_token(&eos_token, stdout);
        goto cleanup;
    }

    struct ast_node * ast = parser_parse(&parser_ctx);

    int exit_code = 0;

    if (parser_ctx.errors.count > 0) {
        error_list_print(&parser_ctx.errors);
    }

    if (parser_ctx.has_error) {
        exit_code = 1;
        goto cleanup;
    }

    if (output_stage == STAGE_AST) {
        if (output_format == FORMAT_DOT) {
            print_ast_dot(ast, stdout);
        } else {
            print_ast(ast, stdout);
        }
        goto cleanup;
    }

    struct ir_context ir_ctx;
    ir_context_init(&ir_ctx, &ctx.pool);

    struct ir_program ir_program;
    ir_program_init(&ir_program, &ctx.pool);

    {
        struct ast_node_list * iterator = ast->content.translation_unit.list;
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
    source_free(&source);
    cclynx_free(&ctx);

    return exit_code;
}


void parse_options(const int argc, const char * argv[])
{
    for(int i = 1; i < argc; ++i) {
        const char * arg = argv[i];

        if (strcmp(arg, "--emit-tokens") == 0) {
            output_stage = STAGE_TOKENS;
            continue;
        }

        if (strcmp(arg, "--emit-ast") == 0) {
            output_stage = STAGE_AST;
            continue;
        }

        if (strcmp(arg, "--format=dot") == 0) {
            output_format = FORMAT_DOT;
            output_format_explicit = true;
            continue;
        }

        if (strcmp(arg, "--format=tree") == 0) {
            output_format = FORMAT_TREE;
            output_format_explicit = true;
            continue;
        }

        if (strcmp(arg, "--emit-ir") == 0) {
            output_stage = STAGE_IR;
            continue;
        }

        if (strcmp(arg, "--emit-asm") == 0) {
            output_stage = STAGE_ASM;
            continue;
        }

        if (strcmp(arg, "--no-warnings") == 0) {
            warning_disable_all(&warning_flags);
            continue;
        }

        if (strcmp(arg, "--help") == 0) {
            show_usage(argv[0], stdout);
            exit(0);
        }

        if (strcmp(arg, "-Wall") == 0) {
            warning_enable_all(&warning_flags);
            continue;
        }

        if (strncmp(arg, "-Wno-", sizeof("-Wno-") - 1) == 0) {
            const char * name = arg + sizeof("-Wno-") - 1;
            if (*name == '\0') {
                cclynx_fatal_error("ERROR: missing warning name after -Wno-\n");
            }
            if (!warning_disable_by_name(&warning_flags, name)) {
                cclynx_fatal_error("ERROR: unknown warning \"%s\"\n", name);
            }
            continue;
        }

        if (strcmp(arg, "-Wtolerant") == 0) {
            warning_apply_tolerant(&warning_flags);
            continue;
        }

        if (strncmp(arg, "--", sizeof("--") - 1) == 0) {
            cclynx_fatal_error("ERROR: unknown option \"%s\"\n", arg);
        }

        if (strncmp(arg, "-W", sizeof("-W") - 1) == 0) {
            cclynx_fatal_error("ERROR: unknown warning option \"%s\"\n", arg);
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
    fprintf(output, "\t--format=tree|dot\n\t    Output format (default: tree).\n\n");
    fprintf(output, "\t--emit-ir\n\t    Produces intermediate representation.\n\n");
    fprintf(output, "\t--emit-asm\n\t    Produces assembly (default).\n\n");
    fprintf(output, "\t--no-warnings\n\t    Suppress all warning messages.\n\n");
    fprintf(output, "\t-Wall\n\t    Enable all warnings.\n\n");
    fprintf(output, "\t-Wno-<name>\n\t    Disable a specific warning or category.\n\n");
    fprintf(output, "\t-Wtolerant\n\t    Suppress noisy warnings (signedness).\n\n");
}
