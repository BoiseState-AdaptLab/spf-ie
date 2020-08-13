void matrix_add(int a, int b, int x[a][b], int y[a][b], int sum[a][b]) {
    int i;
    int j;
    for (i = 0; i < a; i++) {
        for (j = 0; j < b; j++) {
            sum[i][j] = x[i][j] + y[i][j];
        }
    }
}
