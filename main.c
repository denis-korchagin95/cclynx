#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "identifier.h"
#include "symbol.h"
#include "tokenizer.h"
#include "print.h"
#include "parser.h"


int main(int argc, const char * argv[])
{
    const char * example_filename = "./examples/factorial.c";
    FILE * file = fopen(example_filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open file %s\n", example_filename);
        exit(1);
    }

    memory_blob_pool_init(&main_pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
    init_keywords();
    init_symbols();

    struct token * tokens = tokenizer_tokenize_file(file);

    struct parser_context context;
    parser_init_context(&context, tokens);

    struct ast_node * ast = parser_parse(&context);

    print_ast(ast, stdout);

    memory_blob_pool_free(&main_pool, false);

    fclose(file);

    return 0;
}
