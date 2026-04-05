#ifndef CCLYNX_ERROR_H
#define CCLYNX_ERROR_H 1

#include <stdint.h>

struct memory_blob_pool;

struct error_item
{
    char * message;
    struct error_item * next;
};

#define ERROR_LIST_MAX_ERRORS 20

struct error_list
{
    struct error_item * head;
    struct error_item ** tail;
    struct memory_blob_pool * pool;
    unsigned int count;
};

void error_list_init(struct error_list * list, struct memory_blob_pool * pool);
void error_list_add(struct error_list * list, const char * fmt, ...);
void error_list_print(const struct error_list * list);

_Noreturn void cclynx_fatal_error(const char * fmt, ...);

#endif /* CCLYNX_ERROR_H */
