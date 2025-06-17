#ifndef SYMBOL_H
#define SYMBOL_H 1

struct type;

enum symbol_kind
{
    SYMBOL_KIND_TYPE_SPECIFIER = 1,
    SYMBOL_KIND_VARIABLE,
};

struct symbol
{
    struct identifier * identifier;
    struct type * type;
    enum symbol_kind kind;
};

struct symbol_list
{
    struct symbol * symbol;
    struct symbol_list * next;
};

void init_symbols(void);
struct symbol * symbol_lookup(const struct identifier * identifier, enum symbol_kind kind);

#endif /* SYMBOL_H */
