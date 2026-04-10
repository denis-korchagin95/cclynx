// expected return: 95
// wrapper: ./scripts/uint32_wrapper.c
unsigned int main(void) {
    unsigned int x;
    x = 4294967295u;
    return x % 100u;
}
