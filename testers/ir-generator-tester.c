#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "identifier.h"
#include "symbol.h"
#include "tokenizer.h"
#include "print.h"
#include "parser.h"
#include "ir.h"


int main(const int argc, const char * argv[])
{
    if (argc <= 1) {
        fprintf(stderr, "No source given!\n");
        exit(1);
    }

    FILE * file = fopen(argv[1], "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        exit(1);
    }

    memory_blob_pool_init(&main_pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
    init_keywords();
    init_symbols();

    struct token * tokens = tokenizer_tokenize_file(file);

    struct parser_context context;
    parser_init_context(&context, tokens);

    const struct ast_node * ast = parser_parse(&context);

    struct ir_program ir_program;

    ir_program_init(&ir_program);

    ir_program_generate(&ir_program, ast);

    print_ir_program(&ir_program, stdout);

    memory_blob_pool_free(&main_pool, false);

    fclose(file);

    return 0;
}
