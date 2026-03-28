#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "source.h"
#include "errors.h"

void source_load(struct source * source, const char * path)
{
    assert(source != NULL);
    assert(path != NULL);

    FILE * file = fopen(path, "rb");

    if (file == NULL) {
        cclynx_fatal_error("ERROR: cannot open file '%s'\n", path);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char * content = malloc(file_size + 1);

    if (content == NULL) {
        fclose(file);
        cclynx_fatal_error("ERROR: cannot allocate memory for file '%s'\n", path);
    }

    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);

    content[read_size] = '\0';

    source->content = content;
    source->path = (char *)path;
    source->cursor = 0;
    source->size = read_size;
    source->line = 1;
    source->column = 1;
}

int source_get_char(struct source * source)
{
    assert(source != NULL);

    if (source->cursor >= source->size) {
        return EOF;
    }

    int ch = (unsigned char)source->content[source->cursor++];

    if (ch == '\n') {
        source->line++;
        source->column = 1;
    } else {
        source->column++;
    }

    return ch;
}

void source_unget_char(struct source * source, int ch)
{
    assert(source != NULL);

    if (ch == EOF) {
        cclynx_fatal_error("ERROR: cannot unget EOF\n");
    }

    assert(source->cursor > 0);
    source->cursor--;

    if (ch == '\n') {
        source->line--;
        source->column = 1;
        for (size_t i = source->cursor; i > 0; i--) {
            if (source->content[i - 1] == '\n')
                break;
            source->column++;
        }
    } else {
        source->column--;
    }
}

void source_free(struct source * source)
{
    assert(source != NULL);

    free(source->content);
    source->content = NULL;
}
