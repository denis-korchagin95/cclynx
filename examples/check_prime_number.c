int main() {
    int n;
    int i;
    int prime;
    n = 29; // check that N is prime
    i = 2;
    prime = 1;
    while (i < n) {
        if ((n / i) * i == n) {
            prime = 0;
        }
        i = i + 1;
    }
    return prime;
}
