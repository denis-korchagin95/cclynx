// expected return: 45
int main(void) {
    int i;
    int sum;
    i = 0;
    sum = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        if (i == 7) {
            continue;
        }
        sum = sum + i;
    }
    return sum;
}
