// expected return: 2147483647
// wrapper: ./scripts/uint32_wrapper.c
unsigned int main() {
    unsigned int x;
    x = 4294967295u;
    return x / 2u;
}
