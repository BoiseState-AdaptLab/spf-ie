int CSR_SpMV(int a, int N, int A[a], int index[N + 1], int col[a], int x[N], int product[N]) {
    int i;
    int k;
    for (i = 0; i < N; i++) {
        for (k = index[i]; k < index[i + 1]; k++) {
            product[i] += A[k] * x[col[k]];
        }
    }

    return 0;
}
