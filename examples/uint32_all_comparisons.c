// expected return: 5
// wrapper: ./scripts/uint32_wrapper.c
unsigned int main() {
    unsigned int result;
    result = 0u;

    unsigned int a;
    a = 4294967295u;
    unsigned int b;
    b = 1u;

    unsigned int d;
    d = a / 2u;
    // udiv: 4294967295 / 2 = 2147483647
    if (d > 2147483646u) {
        result = result + 1u;
    }

    // unsigned gt: 4294967295 > 1 is true (signed would see -1 > 1 = false)
    if (a > b) {
        result = result + 1u;
    }

    // unsigned lt: 1 < 4294967295 is true
    if (b < a) {
        result = result + 1u;
    }

    unsigned int c;
    c = 10u + 20u;
    c = c - 5u;
    c = c * 2u;
    if (c > 49u) {
        result = result + 1u;
    }

    // eq
    if (c == 50u) {
        result = result + 1u;
    }

    return result;
}
