#include <stdio.h>
#include <stdlib.h>

extern unsigned int _main(void);

int main(int argc, char * argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <expected>\n", argv[0]);
        return 1;
    }

    unsigned int expected = (unsigned int) strtoul(argv[1], NULL, 10);
    unsigned int actual = _main();

    if (actual != expected) {
        fprintf(stderr, "expected %u, got %u\n", expected, actual);
        return 1;
    }

    return 0;
}
