// gcc -O2 -o matrix_generator matrix_generator.c
// ./matrix_generator

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROWS_PER_PROC 50000
#define NPROCS        128
#define NNZ_PER_ROW   50
#define FILENAME      "WS8.mtx"

int main()
{
    srand(12345);

    int N = ROWS_PER_PROC * NPROCS;
    long long nnz = (long long)N * NNZ_PER_ROW;

    FILE *f = fopen(FILENAME, "w");
    if(!f){
        perror("fopen");
        return 1;
    }

    /* Matrix Market header */
    fprintf(f, "%%%%MatrixMarket matrix coordinate real general\n");
    fprintf(f, "%% Synthetic sparse matrix for weak scaling\n");
    fprintf(f, "%d %d %lld\n", N, N, nnz);

    /* For each row, generate:
       - 1 diagonal element
       - NNZ_PER_ROW - 1 random columns
    */
    for(int i = 0; i < N; i++) {

        /* Diagonal (guarantees non-singular structure) */
        fprintf(f, "%d %d %.6f\n", i+1, i+1, 1.0);

        int generated = 1;

        while(generated < NNZ_PER_ROW) {
            int j = rand() % N;

            if(j == i) continue;  // skip diagonal duplicate

            double val = ((double)rand() / RAND_MAX) - 0.5;
            fprintf(f, "%d %d %.6f\n", i+1, j+1, val);
            generated++;
        }
    }

    fclose(f);

    printf("Matrix generated: %s\n", FILENAME);
    printf("Size: %d x %d\n", N, N);
    printf("NNZ per row: %d\n", NNZ_PER_ROW);
    printf("Total NNZ: %lld\n", nnz);

    return 0;
}
