int main() {
    int n;
    int result;
    n = 5;
    result = 1;
    while (n > 1) {
        result = result * n;
        n = n - 1;
    }
    return result;
}
