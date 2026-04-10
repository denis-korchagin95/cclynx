// expected return: 11
// wrapper: ./scripts/int32_wrapper.c
int main(void) {
    int x;
    int result;
    x = 0;
    result = 0;
    if (!x) {
        result = result + 1;
    }
    if (!5) {
        result = result + 100;
    }
    while (!result) {
        result = 99;
    }
    x = 7;
    while (!(x == 17)) {
        result = result + 1;
        x = x + 1;
    }
    return result;
}
