#include <stdio.h>
#include <stdlib.h>

extern int _main(void);

int main(int argc, char * argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <expected>\n", argv[0]);
        return 1;
    }

    int expected = (int) strtol(argv[1], NULL, 10);
    int actual = _main();

    if (actual != expected) {
        fprintf(stderr, "expected %d, got %d\n", expected, actual);
        return 1;
    }

    return 0;
}
