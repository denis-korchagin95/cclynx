#ifndef SYMBOL_H
#define SYMBOL_H 1

enum symbol_kind
{
    SYMBOL_KIND_TYPE_SPECIFIER = 1,
};

struct symbol
{
    struct identifier * identifier;
    struct symbol * next;
    enum symbol_kind kind;
};

void init_symbols(void);
struct symbol * symbol_lookup(const struct identifier * identifier, enum symbol_kind kind);

#endif /* SYMBOL_H */
