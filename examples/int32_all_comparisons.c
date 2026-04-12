// expected return: 11
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

    // signed le: -1 <= 1 is true
    if (a <= b) {
        result = result + 1;
    }

    // le: 1 <= 1 is true (equal case)
    if (b <= 1) {
        result = result + 1;
    }

    // ge: 1 >= -1 is true
    if (b >= a) {
        result = result + 1;
    }

    // ge: 50 >= 50 is true (equal case)
    if (c >= 50) {
        result = result + 1;
    }

    // le (negative): 1 <= -1 is false
    if (b <= a) {
        result = result + 100;
    }

    // ge (negative): -1 >= 1 is false
    if (a >= b) {
        result = result + 100;
    }

    // le (negative): 50 <= 49 is false (strictly greater)
    if (c <= 49) {
        result = result + 100;
    }

    // negative ge: -1 >= 0 is false (strictly less)
    if (a >= 0) {
        result = result + 100;
    }

    // le as value: -1 <= 1 is 1
    result = result + (a <= b);

    // ge as value: 1 >= -1 is 1
    result = result + (b >= a);

    // le as value (negative): 1 <= -1 is 0
    result = result + (b <= a);

    // ge as value (negative): -1 >= 1 is 0
    result = result + (a >= b);

    return result;
}
