// expected return: 11
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

    // gt: 4294967295 > 1 is true (signed would see -1 > 1 = false)
    if (a > b) {
        result = result + 1u;
    }

    // lt: 1 < 4294967295 is true
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

    // le: 1 <= 4294967295 is true
    if (b <= a) {
        result = result + 1u;
    }

    // le: 1 <= 1 is true (equal case)
    if (b <= 1u) {
        result = result + 1u;
    }

    // ge: 4294967295 >= 1 is true
    if (a >= b) {
        result = result + 1u;
    }

    // ge: 50 >= 50 is true (equal case)
    if (c >= 50u) {
        result = result + 1u;
    }

    // le (negative): 4294967295 <= 1 is false
    if (a <= b) {
        result = result + 100u;
    }

    // ge (negative): 1 >= 4294967295 is false
    if (b >= a) {
        result = result + 100u;
    }

    // le (negative): 50 <= 49 is false (strictly greater)
    if (c <= 49u) {
        result = result + 100u;
    }

    // ge (negative): 1 >= 2 is false (strictly less)
    if (b >= 2u) {
        result = result + 100u;
    }

    // le as value: 1 <= 4294967295 is 1
    result = result + (b <= a);

    // ge as value: 4294967295 >= 1 is 1
    result = result + (a >= b);

    // le as value (negative): 4294967295 <= 1 is 0
    result = result + (a <= b);

    // ge as value (negative): 1 >= 4294967295 is 0
    result = result + (b >= a);

    return result;
}
