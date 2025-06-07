int main() {
    int base;
    int exp;
    int result;
    base = 2;
    exp = 5;
    result = 1;
    while (exp > 0) {
        result = result * base;
        exp = exp - 1;
    }
    return result;
}
