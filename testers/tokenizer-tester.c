#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "tokenizer.h"
#include "print.h"
#include "symbol.h"


int main(int argc, const char * argv[])
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
    init_builtin_symbols();

    for (;;) {
        struct token * token = tokenizer_get_one_token(file);

        print_token(token, stdout);

        if (token->kind == TOKEN_KIND_EOF) {
            break;
        }
    }

    memory_blob_pool_free(&main_pool, false);

    fclose(file);

    return 0;
}
