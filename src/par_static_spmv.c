/********************************************************************************************
 * Description:
 *    High-performance parallel Sparse Matrix-Vector multiplication (SpMV) benchmark
 *    using the Compressed Sparse Row (CSR) format and OpenMP. 
 *
 *    This program:
 *        1. Reads a sparse matrix stored in Matrix Market (.mtx) coordinate format.
 *        2. Converts it into CSR representation, expanding symmetric matrices when needed.
 *        3. Reads an input vector from a Matrix Market file.
 *        4. Performs y = A * x using a CSR kernel with OpenMP parallelization and static scheduling.
 *        5. Measures total kernel execution time with wall-clock precision.
 *
 * Matrix Format:
 *    Input matrix must be in:
 *        Matrix Market (.mtx), coordinate, real, general/symmetric
 *
 * Vector Format:
 *    Input vector must be a column vector in Matrix Market array format:
 *        N × 1 entries
 *
 * Compilation:
 *        gcc -g -Wall -fopenmp -o par_static_spmv par_static_spmv.c mmio.c
 *
 * Execution:
 *        ./par_static_spmv matrix.mtx vector.mtx <threads> <chunk_size>
 *
 * Output:
 *        - Elapsed time (seconds)
 *
 * Requirements:
 *        - mmio.c/mmio.h for Matrix Market parsing
 ********************************************************************************************/

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "mmio.h"

// ------------------ READ MATRIX (CSR) ------------------
void read_matrix_csr(const char *fname, int *M_out, int *N_out, int *nz_out,
                     double **values_out, int **col_idx_out, int **row_ptr_out)
{
    FILE *f;
    MM_typecode matcode;
    int M, N, nz;

    /* Open MatrixMarket file */
    if ((f = fopen(fname, "r")) == NULL){
        fprintf(stderr, "Cannot open %s\n", fname);
        exit(1);
    }

    /* Read MatrixMarket header */
    if (mm_read_banner(f, &matcode) != 0){
        fprintf(stderr, "Error reading banner\n");
        exit(1);
    }

    /* Read matrix dimensions and number of nonzeros */
    if (mm_read_mtx_crd_size(f, &M, &N, &nz) != 0){
        fprintf(stderr, "Error reading size\n");
        exit(1);
    }

    /* Check if matrix is symmetric */
    int symmetric = mm_is_symmetric(matcode);

    /* Worst-case allocation:
       If the matrix is symmetric, every entry (i,j) may need a matching (j,i),
       so reserve 2*nz. Otherwise allocate exactly nz. */
    int alloc = symmetric ? (2 * nz) : nz;

    int *I = malloc(alloc * sizeof(int));
    int *J = malloc(alloc * sizeof(int));
    double *val = malloc(alloc * sizeof(double));

    int count = 0;

    /* Read all entries from file */
    for (int k = 0; k < nz; k++){
        int r, c;
        double v;
        fscanf(f, "%d %d %lf", &r, &c, &v);

        /* Convert from 1-based to 0-based indexing */
        r--; 
        c--;

        /* Store entry as provided in the file */
        I[count] = r;
        J[count] = c;
        val[count] = v;
        count++;

        /* If matrix is symmetric, also insert the mirrored entry (c,r)
           except when it is on the diagonal (r == c). */
        if (symmetric && r != c){
            I[count] = c;
            J[count] = r;
            val[count] = v;
            count++;
        }
    }
    fclose(f);

    /* Now `count` is the true number of entries after symmetric expansion */
    nz = count;

    double *values = malloc(nz * sizeof(double));
    int *col_idx = malloc(nz * sizeof(int));
    int *row_ptr = calloc(M + 1, sizeof(int));

    /* Count how many entries go into each row */
    for (int i = 0; i < nz; i++)
        row_ptr[I[i] + 1]++;

    /* Convert counts into prefix-sum to build CSR row_ptr */
    for (int i = 0; i < M; i++)
        row_ptr[i + 1] += row_ptr[i];

    /* Temporary counters used for placing each entry in its correct CSR position */
    int *offset = calloc(M, sizeof(int));

    /* Fill CSR structure */
    for (int i = 0; i < nz; i++){
        int row = I[i];
        int pos = row_ptr[row] + offset[row];

        values[pos] = val[i];
        col_idx[pos] = J[i];
        offset[row]++;
    }

    /* Free temporary arrays */
    free(I);
    free(J);
    free(val);
    free(offset);

    /* Output final CSR components */
    *M_out = M;
    *N_out = N;
    *nz_out = nz;
    *values_out = values;
    *col_idx_out = col_idx;
    *row_ptr_out = row_ptr;
}
// read_matrix_csr


// ------------------ READ VECTOR (.mtx format) ------------------
double *read_vector_mtx(const char *fname, int *length_out) {
    FILE *f;
    MM_typecode matcode;
    int M, N;

    if((f = fopen(fname, "r")) == NULL){
        fprintf(stderr, "Error opening vector file %s\n", fname);
        exit(1);
    }

    if(mm_read_banner(f, &matcode) != 0){
        fprintf(stderr, "Could not process Matrix Market banner.\n");
        exit(1);
    }

    if (mm_read_mtx_array_size(f, &M, &N) != 0) {
        fprintf(stderr, "Could not read vector dimensions.\n");
        exit(1);
    }

    if (N != 1) {
        fprintf(stderr, "Error: file %s is not a column vector (cols=%d)\n", fname, N);
        exit(1);
    }

    double *vec = malloc(M * sizeof(double));
    if(!vec){ fprintf(stderr, "Memory allocation failed for vector\n"); exit(1); }

    for(int i = 0; i < M; i++) {
        if(fscanf(f, "%lf", &vec[i]) != 1) {
            fprintf(stderr, "Error reading value %d from %s\n", i, fname);
            exit(1);
        }
    }

    fclose(f);
    *length_out = M;
    return vec;
}
// read_vector_mtx


// ------------------ SPARSE MATRIX × VECTOR ------------------
double csr_vector_multiply(int M, double *values, int *col_idx, int *row_ptr, double *x, double *y, int thread_count, int chunk_size){
    double start = omp_get_wtime();
    #pragma omp parallel for num_threads(thread_count) schedule(static, chunk_size)
    for(int i = 0; i < M; i++){
        double sum = 0.0;
        for(int j = row_ptr[i]; j < row_ptr[i + 1]; j++){
            sum += values[j] * x[col_idx[j]];
        }
        y[i] = sum;
    }
    double end = omp_get_wtime();
    return end - start;
}
// csr_vector_multiply


// ------------------ MAIN ------------------
int main(int argc, char *argv[]) {
    if(argc < 5){
        printf("Usage: %s matrix.mtx vector.txt #threads #chunk_size\n", argv[0]);
        return 1;
    }

    int M, N, nz;
    double *values;
    int *col_idx, *row_ptr;
    double elapsed;
    int thread_count = strtol(argv[3], NULL, 10);
    int chunk_size = strtol(argv[4], NULL, 10);

    // Read CSR matrix
    read_matrix_csr(argv[1], &M, &N, &nz, &values, &col_idx, &row_ptr);

    // Read vector x from file
    int vec_len;
    double *x = read_vector_mtx(argv[2], &vec_len);
    if (vec_len != N) {
        fprintf(stderr, "Error: vector length (%d) does not match matrix columns (%d)\n", vec_len, N);
        exit(1);
    }

    // Allocate output vector y and initialize to zero
    double *y = (double*) calloc(M, sizeof(double));
    if(!y){ fprintf(stderr,"Error allocating output vector\n"); return 1; }

    // Multiply
    elapsed = csr_vector_multiply(M, values, col_idx, row_ptr, x, y, thread_count, chunk_size);

    printf("\nElapsed time = %e seconds with %d threads and chunksize %d\n\n", elapsed, thread_count, chunk_size);

    // Cleanup
    free(values); free(col_idx); free(row_ptr); free(x); free(y);

    return 0;
}