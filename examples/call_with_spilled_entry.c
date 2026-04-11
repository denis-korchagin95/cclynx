// expected return: 45
int identity(int x) {
    return x;
}

int main(void) {
    return 1 + identity(2 + (3 + (4 + (5 + (6 + (7 + (8 + 9)))))));
}
