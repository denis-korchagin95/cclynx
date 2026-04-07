// expected return: 3
int main(void) {
    int i;
    int j;
    int sum;
    i = 0;
    sum = 0;
    while (i < 3) {
        j = 0;
        while (j < i) {
            sum = sum + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    return sum;
}
