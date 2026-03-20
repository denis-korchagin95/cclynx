// expected return: 4
int main() {
    float x;
    float threshold;
    int count;
    x = 10.0;
    threshold = 1.0;
    count = 0;
    while (x > threshold) {
        x = x / 2.0;
        count = count + 1;
    }
    return count;
}
