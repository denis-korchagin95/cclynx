#include <string.h>

#include "warning.h"


struct warning_entry {
    const char * name;
    enum warning_category category;
};

static const struct warning_entry warning_table[WARNING_COUNT] = {
    [WARNING_UNUSED_VARIABLE]          = { "unused-variable",          WARNING_CATEGORY_UNUSED },
    [WARNING_UNUSED_PARAMETER]         = { "unused-parameter",         WARNING_CATEGORY_UNUSED },
    [WARNING_EMPTY_COMPOUND_STATEMENT] = { "empty-compound-statement", WARNING_CATEGORY_EMPTY_BODY },
    [WARNING_EMPTY_IF_BODY]            = { "empty-if-body",            WARNING_CATEGORY_EMPTY_BODY },
    [WARNING_EMPTY_ELSE_BODY]          = { "empty-else-body",          WARNING_CATEGORY_EMPTY_BODY },
    [WARNING_EMPTY_WHILE_BODY]         = { "empty-while-body",         WARNING_CATEGORY_EMPTY_BODY },
    [WARNING_EMPTY_FUNCTION_BODY]      = { "empty-function-body",      WARNING_CATEGORY_EMPTY_BODY },
    [WARNING_MISSING_RETURN]           = { "missing-return",           WARNING_CATEGORY_MISSING },
    [WARNING_UNSPECIFIED_PARAMETERS]   = { "unspecified-parameters",   WARNING_CATEGORY_NONE },
    [WARNING_SIGN_CONVERSION]          = { "sign-conversion",          WARNING_CATEGORY_SIGNEDNESS },
};

static const char * category_names[WARNING_CATEGORY_COUNT] = {
    [WARNING_CATEGORY_NONE]       = NULL,
    [WARNING_CATEGORY_UNUSED]     = "unused",
    [WARNING_CATEGORY_MISSING]    = "missing",
    [WARNING_CATEGORY_EMPTY_BODY] = "empty-body",
    [WARNING_CATEGORY_SIGNEDNESS] = "signedness",
};


void warning_init_default(struct warning_flags * flags)
{
    for (int i = 0; i < WARNING_COUNT; ++i) {
        flags->enabled[i] = true;
    }
    warning_apply_tolerant(flags);
}

void warning_enable_all(struct warning_flags * flags)
{
    for (int i = 0; i < WARNING_COUNT; ++i) {
        flags->enabled[i] = true;
    }
}

void warning_disable_all(struct warning_flags * flags)
{
    for (int i = 0; i < WARNING_COUNT; ++i) {
        flags->enabled[i] = false;
    }
}

void warning_apply_tolerant(struct warning_flags * flags)
{
    for (int i = 0; i < WARNING_COUNT; ++i) {
        if (warning_table[i].category == WARNING_CATEGORY_SIGNEDNESS) {
            flags->enabled[i] = false;
        }
    }
}

bool warning_disable_by_name(struct warning_flags * flags, const char * name)
{
    for (int i = 0; i < WARNING_COUNT; ++i) {
        if (strcmp(warning_table[i].name, name) == 0) {
            flags->enabled[i] = false;
            return true;
        }
    }

    for (int i = 0; i < WARNING_CATEGORY_COUNT; ++i) {
        if (category_names[i] != NULL && strcmp(category_names[i], name) == 0) {
            for (int j = 0; j < WARNING_COUNT; ++j) {
                if (warning_table[j].category == (enum warning_category)i) {
                    flags->enabled[j] = false;
                }
            }
            return true;
        }
    }

    return false;
}

bool warning_is_enabled(const struct warning_flags * flags, enum warning_code code)
{
    return flags->enabled[code];
}
