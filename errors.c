#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "allocator.h"


void error_list_init(struct error_list * list, struct memory_blob_pool * pool)
{
    assert(list != NULL);
    assert(pool != NULL);

    list->head = NULL;
    list->tail = &list->head;
    list->pool = pool;
    list->count = 0;
}

void error_list_add(struct error_list * list, const char * fmt, ...)
{
    assert(list != NULL);
    assert(fmt != NULL);

    list->count++;

    if (list->count > ERROR_LIST_MAX_ERRORS) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    char * message = memory_blob_pool_alloc(list->pool, len + 1);
    vsnprintf(message, len + 1, fmt, args);
    va_end(args);

    struct error_item * entry = memory_blob_pool_alloc(list->pool, sizeof(struct error_item));
    entry->message = message;
    entry->next = NULL;

    *list->tail = entry;
    list->tail = &entry->next;
}

void error_list_print(const struct error_list * list)
{
    assert(list != NULL);

    const struct error_item * entry = list->head;
    while (entry != NULL) {
        fprintf(stderr, "%s", entry->message);
        entry = entry->next;
    }

    if (list->count > ERROR_LIST_MAX_ERRORS) {
        fprintf(stderr, "too many errors, %u not shown\n", list->count - ERROR_LIST_MAX_ERRORS);
    }

    fflush(stderr);
}

_Noreturn void cclynx_fatal_error(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
