/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        for (int j = 0; j < M; j += 8) {
            for (int i = 0; i < N; ++i) {
                int tmp = A[i][j];
                int tmp2 = A[i][j + 1];
                int tmp3 = A[i][j + 2];
                int tmp4 = A[i][j + 3];
                int tmp5 = A[i][j + 4];
                int tmp6 = A[i][j + 5];
                int tmp7 = A[i][j + 6];
                int tmp8 = A[i][j + 7];
                
                B[j][i] = tmp;
                B[j + 1][i] = tmp2;
                B[j + 2][i] = tmp3;
                B[j + 3][i] = tmp4;
                B[j + 4][i] = tmp5;
                B[j + 5][i] = tmp6;
                B[j + 6][i] = tmp7;
                B[j + 7][i] = tmp8;
            }
        }
    } else if (M == 64 && N == 64) {
        for (int i = 0; i < M; i += 8) {
            for (int j = 0; j < N; j += 8) {
                for (int k = i; k < i + 4; ++k) {
                    int tmp = A[k][j], tmp1 = A[k][j + 1], tmp2 = A[k][j + 2], tmp3 = A[k][j + 3];
                    int tmp4 = A[k][j + 4], tmp5 = A[k][j + 5], tmp6= A[k][j + 6], tmp7 = A[k][j + 7];

                    B[j][k] = tmp; B[j + 1][k] = tmp1; B[j + 2][k] = tmp2; B[j + 3][k] = tmp3;
                    B[j][k + 4] = tmp4; B[j + 1][k + 4] = tmp5; B[j + 2][k + 4] = tmp6; B[j + 3][k + 4] = tmp7;   
                }
                for (int k = j; k < j + 4; ++k) { // why? 画个图对照着模拟一遍会很有帮助
                    int tmp4 = A[i + 4][k], tmp5 = A[i + 5][k], tmp6 = A[i + 6][k], tmp7 = A[i + 7][k];
                    int tmp = B[k][i + 4], tmp1 = B[k][i + 5], tmp2 = B[k][i + 6], tmp3 = B[k][i + 7];

                    B[k][i + 4] = tmp4; B[k][i + 5] = tmp5; B[k][i + 6] = tmp6; B[k][i + 7] = tmp7;
                    B[k + 4][i] = tmp; B[k + 4][i + 1] = tmp1; B[k + 4][i + 2] = tmp2; B[k + 4][i + 3] = tmp3;
                }
                for (int k = i + 4; k < i + 8; ++k) {
                    int tmp = A[k][j + 4], tmp1 = A[k][j + 5], tmp2 = A[k][j + 6], tmp3 = A[k][j + 7];
                    B[j + 4][k] = tmp; B[j + 5][k] = tmp1; B[j + 6][k] = tmp2; B[j + 7][k] = tmp3;
                }
            }
        }
    } else {
        for (int j = 0; j < M; j += 8) {
            for (int i = 0; i < N; ++i) {
                int tmp = A[i][j];
                int tmp2 = A[i][j + 1];
                int tmp3 = A[i][j + 2];
                int tmp4 = A[i][j + 3];
                int tmp5 = A[i][j + 4];
                int tmp6 = A[i][j + 5];
                int tmp7 = A[i][j + 6];
                int tmp8 = A[i][j + 7];
                
                B[j][i] = tmp;
                B[j + 1][i] = tmp2;
                B[j + 2][i] = tmp3;
                B[j + 3][i] = tmp4;
                B[j + 4][i] = tmp5;
                B[j + 5][i] = tmp6;
                B[j + 6][i] = tmp7;
                B[j + 7][i] = tmp8;
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{   
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

