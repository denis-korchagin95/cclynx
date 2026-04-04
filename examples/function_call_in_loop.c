// expected return: 10
int add(int a, int b) {
    return a + b;
}

int main(void) {
    int i;
    i = 0;
    while (i < 10) {
        i = add(i, 1);
    }
    return i;
}
