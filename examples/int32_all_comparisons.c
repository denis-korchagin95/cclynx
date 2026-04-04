// expected return: 5
// wrapper: ./scripts/int32_wrapper.c
int main() {
    int result;
    result = 0;

    int a;
    a = 0 - 1;
    int b;
    b = 1;

    int d;
    d = a / 2;
    // sdiv: -1 / 2 = 0 (truncated toward zero)
    if (d > 0 - 1) {
        result = result + 1;
    }

    // signed gt: 1 > -1 is true
    if (b > a) {
        result = result + 1;
    }

    // signed lt: -1 < 1 is true
    if (a < b) {
        result = result + 1;
    }

    int c;
    c = 10 + 20;
    c = c - 5;
    c = c * 2;
    if (c > 49) {
        result = result + 1;
    }

    // eq
    if (c == 50) {
        result = result + 1;
    }

    return result;
}
