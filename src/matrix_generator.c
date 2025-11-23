/***************************************************************************************
 * Description:
 *     This program generates a random sparse matrix in Matrix Market (.mtx) format
 *     using coordinate storage (row, column, value). The matrix is generated with
 *     user-defined dimensions and a fixed density of non-zero values.
 *
 *     The generated file will contain:
 *         - Matrix Market header
 *         - Matrix dimensions (ROWS x COLS)
 *         - Number of non-zero elements (NNZ)
 *         - Triples of the form: row column value
 *
 * Compilation:
 *         gcc -O2 -o matrix_generator matrix_generator.c
 *
 * Execution:
 *         ./matrix_generator
 *
 * Output:
 *         Creates a file named:
 *             m.mtx
 *
 * Parameters (modifiable via #define):
 *         ROWS    - Number of rows in the matrix
 *         COLS    - Number of columns in the matrix
 *         DENSITY - Fraction of non-zero elements (0 < DENSITY ≤ 1)
 *
 * Notes:
 *         - Generates values uniformly in the range [0,1).
 **************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 15000       // Number of rows
#define COLS 15000       // Number of columns
#define DENSITY 0.49     // Fraction of non-zero elements (0 < DENSITY <= 1)
#define FILENAME "m.mtx"

int main() {
    srand(time(NULL)); // Initialize random seed

    // Calculate total number of non-zero elements
    long long total_elements = (long long)ROWS * COLS;
    long long nnz = (long long)(total_elements * DENSITY);

    FILE *f = fopen(FILENAME, "w");
    if(!f){
        fprintf(stderr, "Error opening file %s\n", FILENAME);
        return 1;
    }

    // Correct Matrix Market header
    fputs("%%MatrixMarket matrix coordinate real general\n", f);
    fprintf(f, "%% Random sparse matrix %dx%d with %.2f density\n", ROWS, COLS, DENSITY);
    fprintf(f, "%d %d %lld\n", ROWS, COLS, nnz);

    // Generate random entries
    for(long long k = 0; k < nnz; k++){
        int i = rand() % ROWS + 1; // 1-based indexing for Matrix Market
        int j = rand() % COLS + 1;
        double val = (double)rand() / RAND_MAX; // values in [0,1)
        fprintf(f, "%d %d %.6f\n", i, j, val);
    }

    fclose(f);

    printf("Matrix generated: %s\n", FILENAME);
    printf("Dimensions: %d x %d, Non-zeros: %lld\n", ROWS, COLS, nnz);

    return 0;
}