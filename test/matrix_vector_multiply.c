int mvm(int a, int b, int x[a][b], int c, int y[c], int product[a]) {
    if (b != c) return 1;

    int i;
    int j;
    for (i = 0; i < a; i++) {
        product[i] = 0;
        for (j = 0; j < b; j++) {
            product[i] += x[i][j] * y[j];
        }
    }

    return 0;
}
