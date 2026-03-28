#ifndef CCLYNX_SOURCE_H
#define CCLYNX_SOURCE_H 1

#include <stddef.h>
#include <stdint.h>

struct source_span
{
    uint32_t offset;
    uint32_t length;
};

struct source
{
    char * content;
    char * path;
    size_t cursor;
    size_t size;
    uint32_t line;
    uint32_t column;
};

void source_load(struct source * source, const char * path);
int source_get_char(struct source * source);
void source_unget_char(struct source * source, int ch);
void source_free(struct source * source);

#endif /* CCLYNX_SOURCE_H */