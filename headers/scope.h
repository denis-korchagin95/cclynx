#ifndef CCLYNX_SCOPE_H
#define CCLYNX_SCOPE_H 1

struct scope
{
    struct scope * enclosing;
    struct symbol_list * symbols;
};

extern struct scope global_scope;

#endif /* CCLYNX_SCOPE_H */
