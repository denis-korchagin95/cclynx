#ifndef CCLYNX_WARNING_H
#define CCLYNX_WARNING_H 1

#include <stdbool.h>

enum warning_code {
    WARNING_UNUSED_VARIABLE,
    WARNING_UNUSED_PARAMETER,
    WARNING_EMPTY_COMPOUND_STATEMENT,
    WARNING_EMPTY_IF_BODY,
    WARNING_EMPTY_ELSE_BODY,
    WARNING_EMPTY_WHILE_BODY,
    WARNING_EMPTY_FUNCTION_BODY,
    WARNING_MISSING_RETURN,
    WARNING_UNSPECIFIED_PARAMETERS,
    WARNING_SIGN_CONVERSION,
    WARNING_COUNT
};

enum warning_category {
    WARNING_CATEGORY_NONE,
    WARNING_CATEGORY_UNUSED,
    WARNING_CATEGORY_MISSING,
    WARNING_CATEGORY_EMPTY_BODY,
    WARNING_CATEGORY_SIGNEDNESS,
    WARNING_CATEGORY_COUNT
};

struct warning_flags {
    bool enabled[WARNING_COUNT];
};

void warning_init_default(struct warning_flags * flags);
void warning_enable_all(struct warning_flags * flags);
void warning_disable_all(struct warning_flags * flags);
void warning_apply_tolerant(struct warning_flags * flags);
bool warning_disable_by_name(struct warning_flags * flags, const char * name);
bool warning_is_enabled(const struct warning_flags * flags, enum warning_code code);

#endif /* CCLYNX_WARNING_H */
