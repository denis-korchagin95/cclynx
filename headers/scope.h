#ifndef SCOPE_H
#define SCOPE_H 1

struct scope
{
    struct scope * enclosing;
    struct symbol_list * symbols;
};

extern struct scope global_scope;

#endif /* SCOPE_H */
