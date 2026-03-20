#include <stdlib.h>

extern int _main(void);

int main(void)
{
    int result = _main();
    exit(result);
}
