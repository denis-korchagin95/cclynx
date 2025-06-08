#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "allocator.h"
#include "identifier.h"
#include "symbol.h"
#include "tokenizer.h"
#include "print.h"


int main(int argc, const char * argv[])
{
    const char * example_filename = "./examples/factorial.c";
    FILE * file = fopen(example_filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open file %s\n", example_filename);
        exit(1);
    }

    memory_blob_pool_init(&main_pool, DEFAULT_MEMORY_BLOB_SIZE, DEFAULT_MEMORY_BLOB_ALIGNMENT);
    init_builtin_symbols();

    struct token * tokens = tokenizer_tokenize_file(file);

    struct token * it = tokens;

    while (it != &eos_token) {
        print_token(it, stdout);
        it = it->next;
    }

    memory_blob_pool_free(&main_pool, false);

    fclose(file);

    return 0;
}
