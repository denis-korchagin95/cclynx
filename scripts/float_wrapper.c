#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern float _test(void) __asm__("_test");

int main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <expected_float>\n", argv[0]);
        return 1;
    }

    float expected = strtof(argv[1], NULL);
    float actual = _test();

    if (fabsf(actual - expected) < 0.001f) {
        return 0;
    }

    fprintf(stderr, "FAIL: expected %f, got %f\n", expected, actual);
    return 1;
}
