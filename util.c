#include "util.h"


size_t align_up(const size_t size, const size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}
