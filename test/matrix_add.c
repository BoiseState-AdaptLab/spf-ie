/**
BEGIN_EXPECTED
A
    S0: int i;
    S1: int j;
    S2: sum[i][j] = x[i][j] + y[i][j];
B
    S0: {[]}
    S1: {[]}
    S2: {[i,j]: 0 <= i < a and 0 <= j <b}
C
    S0: {[]->[0,0,0,0,0]}
    S1: {[]->[1,0,0,0,0]}
    S2: {[i,j]->[2,i,0,j,0]}
END_EXPECTED
 */

// perform matrix addition
void matrix_add(int a, int b, int x[a][b], int y[a][b], int sum[a][b]) {
    int i;
    int j;
    for (i = 0; i < a; i++) {
        for (j = 0; j < b; j++) {
            sum[i][j] = x[i][j] + y[i][j];
        }
    }
}

// #include <stdio.h>

// int main(void) {
//     int a = 2;
//     int b = 3;
//     int x[2][3] = {
//         {2, 3, 5},
//         {3, 7, 9}
//     };
//     int y[2][3] = {
//         {10, 31, 15},
//         {17, 7, 16}
//     };
//     int sum[2][3];
//     matrix_add(a, b, x, y, sum);
//     for (int i = 0; i < a; i++) {
//         for (int j = 0; j < b; j++) {
//             printf("%d ", sum[i][j]);
//         }
//         printf("\n");
//     }
// }
