int main() {
    int sum;
    int i;
    sum = 0;
    i = 1;
    while (i < 10) {
        if (i == 5) {
            sum = sum + 100;
        }
        if (i != 3) {
            sum = sum + i;
        }
        i = i + 1;
    }
    return sum;
}
