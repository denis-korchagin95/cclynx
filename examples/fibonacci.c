int main() {
    int n;
    int a;
    int b;
    int temp;
    n = 10;
    a = 0;
    b = 1;
    while (n > 0) {
        temp = a + b;
        a = b;
        b = temp;
        n = n - 1;
    }
    return a;
}
