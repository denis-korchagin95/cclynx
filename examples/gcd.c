int main() {
    int a;
    int b;
    int t;
    a = 48;
    b = 18;
    while (b != 0) {
        t = b;
        b = a - (a / b) * b;
        a = t;
    }
    return a;
}
