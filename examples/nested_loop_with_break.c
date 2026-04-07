// expected return: 12
int main(void) {
    int i;
    int j;
    int n;
    i = 0;
    n = 0;
    while (i < 3) {
        j = 0;
        while (j < 10) {
            if (j == 4) {
                break;
            }
            n = n + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    return n;
}
