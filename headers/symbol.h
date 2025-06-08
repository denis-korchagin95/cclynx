#ifndef SYMBOL_H
#define SYMBOL_H 1

enum symbol_kind
{
    SYMBOL_KIND_KEYWORD = 1,
};

struct symbol
{
    struct identifier * identifier;
    struct symbol * next;
    enum symbol_kind kind;
};

void init_builtin_symbols(void);
struct symbol * symbol_lookup(struct identifier * identifier, enum symbol_kind kind);

#endif /* SYMBOL_H */
