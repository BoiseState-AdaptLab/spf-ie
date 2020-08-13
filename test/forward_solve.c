int forward_solve(int n, int l[n][n], double b[n], double x[n]) {
    int i;
    for (i = 0; i < n; i++) {
        x[i] = b[i];
    }

    int j;
    for (j = 0; j < n; j++) {
        x[j] /= l[j][j];
        for (i = j + 1; i < n; i++) {
            if (l[i][j]) x[i] -= l[i][j] * x[j];
        }
    }

    return 0;
}
