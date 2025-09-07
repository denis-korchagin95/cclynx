#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "identifier.h"
#include "symbol.h"
#include "tokenizer.h"
#include "print.h"
#include "parser.h"
#include "ir.h"
#include "target-arm64.h"


const char * source_filename = NULL;
bool show_tokens = false;
bool show_ast = false;
bool show_ir = false;

static void parse_options(const int argc, const char * argv[]);
static void show_usage(const char * program_name, FILE * output);


int main(int argc, const char * argv[])
{
    parse_options(argc, argv);

    for (int i = 1; i < argc; ++i) {
        source_filename = argv[i];

        if (strncmp(source_filename, "--", sizeof("--") - 1) == 0) {
            continue;
        }

        break;
    }

    if (source_filename == NULL) {
        fprintf(stderr, "No source given!\n");
        exit(1);
    }

    FILE * source = fopen(source_filename, "r");

    if (source == NULL) {
        fprintf(stderr, "Could not open file %s\n", source_filename);
        exit(1);
    }

    memory_blob_pool_init(&main_pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
    init_keywords();
    init_symbols();

    struct token * tokens = tokenizer_tokenize_file(source);

    fclose(source);

    struct parser_context context;
    parser_init_context(&context, tokens);

    if (show_tokens) {
        struct token * it = tokens;
        while (it != &eos_token) {
            print_token(it, stdout);
            it = it->next;
        }
        exit(0);
    }

    struct ast_node * ast = parser_parse(&context);

    if (show_ast) {
        print_ast(ast, stdout);
        exit(0);
    }

    struct ir_program ir_program;
    ir_program_init(&ir_program);

    ir_program_generate(&ir_program, ast);

    if (show_ir) {
        print_ir_program(&ir_program, stdout);
        exit(0);
    }

    target_arm64_generate(&ir_program, stdout);

    memory_blob_pool_free(&main_pool, false);

    return 0;
}


void parse_options(const int argc, const char * argv[])
{
    for(int i = 1; i < argc; ++i) {
        const char * arg = argv[i];

        if (strncmp(arg, "--show-tokens", sizeof("--show-tokens") - 1) == 0) {
            show_tokens = true;
            continue;
        }

        if (strncmp(arg, "--show-ast", sizeof("--show-ast") - 1) == 0) {
            show_ast = true;
            continue;
        }

        if (strncmp(arg, "--show-ir", sizeof("--show-ir") - 1) == 0) {
            show_ir = true;
            continue;
        }

        if (strncmp(arg, "--help", sizeof("--help") - 1) == 0) {
            show_usage(argv[0], stdout);
            exit(0);
        }

        if (strncmp(arg, "--", sizeof("--") - 1) == 0) {
            fprintf(stderr, "ERROR: unknown option \"%s\"\n", arg);
            exit(1);
        }
    }
}

void show_usage(const char * program_name, FILE * output)
{
    fprintf(output, "Usage: %s [options] path\n\n\n", program_name);
    fprintf(output, "OPTIONS\n");
    fprintf(output, "\t--help\n\t    Show this message.\n\n");
    fprintf(output, "\t--show-tokens\n\t    Print tokens from lexer.\n\n");
    fprintf(output, "\t--show-ast\n\t    Print abstract syntax tree.\n\n");
    fprintf(output, "\t--show-ir\n\t    Print intermediate representation.\n\n");
}
