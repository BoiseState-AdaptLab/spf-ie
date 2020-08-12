int asdf(int a) {
    int i;
    int x;
    for (i = 0; i < 3; i = i + 1) {
        x = i;
        if (i > 1) {
            x = x + 5;
        }
    }
    return x;
}

int main(void) {
    int i;
    int j;
    int x;
    int M = 5;
    for (i = 0; i < 3; i++) {
        x = i;
        if (i > 1) {
            x = x + 5;
        }
        asdf(x);
        for (j = 1; j < M; j++) {
            if (i == 0) {
                x *= j;
            }
        }
    }
    return x;
}

