/********************************************************************************************
 * Description:
 *    High-performance parallel Sparse Matrix-Vector multiplication (SpMV) benchmark
 *    using the Compressed Sparse Row (CSR) format and OpenMP. 
 *
 *    This program:
 *        1. Reads a sparse matrix stored in Matrix Market (.mtx) coordinate format.
 *        2. Converts it into CSR representation, expanding symmetric matrices when needed.
 *        3. Reads an input vector from a Matrix Market file.
 *        4. Performs y = A * x using a manually optimized CSR kernel featuring:
 *               - Loop unrolling (strengthening ILP)
 *               - Aligned memory accesses (64-byte)
 *               - OpenMP parallelization with guided scheduling and tunable chunk size and threads
 *        5. Measures total kernel execution time with wall-clock precision.
 *
 * Matrix Format:
 *    Input matrix must be in:
 *        Matrix Market (.mtx), coordinate, real, general/symmetric
 *
 *    CSR arrays generated:
 *        values    - non-zero values (double)
 *        col_idx   - corresponding column indices
 *        row_ptr   - prefix-sum pointer array (size M+1)
 *
 * Vector Format:
 *    Input vector must be a column vector in Matrix Market array format:
 *        N × 1 entries
 *
 * Compilation:
 *        gcc -O3 -march=native -o par_guided_imp_spmv par_guided_imp_spmv.c mmio.c -lm -fopenmp
 *
 * Execution:
 *        ./par_guided_imp_spmv matrix.mtx vector.mtx <threads> <chunk_size>
 *
 * Output:
 *        - Elapsed time (seconds)
 *        - Computed output vector 'y' in memory (not written to file)
 *
 * Requirements:
 *        - mmio.c/mmio.h for Matrix Market parsing
 ********************************************************************************************/

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <omp.h>
#include "mmio.h"

/* ---------- Portable aligned memory allocation (Windows + Linux) ---------- */
/* This function allocates memory aligned to 'align' bytes. 
   On Windows it uses _aligned_malloc, on Linux posix_memalign. */
static void *alloc_aligned(size_t bytes, size_t align){
    void *ptr = NULL;  // Pointer to store allocated memory

#if defined(_WIN32)
    // Windows: allocate aligned memory
    ptr = _aligned_malloc(bytes, align);
    if (!ptr) {
        fprintf(stderr, "Error: _aligned_malloc failed\n");
        exit(1);
    }

#else
    // Linux/POSIX: allocate aligned memory
    // posix_memalign requires pointer to pointer, alignment, and size
    if (posix_memalign(&ptr, align, bytes) != 0) {
        fprintf(stderr, "Error: posix_memalign failed\n");
        exit(1);
    }
#endif

    return ptr;  // Return pointer to aligned memory
}

/* Read Matrix Market (.mtx) in coordinate format and convert to CSR.
   This version handles "symmetric" MatrixMarket matrices by expanding
   off-diagonal entries, and allocates aligned CSR arrays using alloc_aligned. */
void read_matrix_csr(const char *fname, int *M_out, int *N_out, int *nz_out,
                     double **values_out, int **col_idx_out, int **row_ptr_out) {
    FILE *f;
    MM_typecode matcode;
    int M, N, nz;

    /* Open file and read header + size */
    if ((f = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "Error: unable to open file %s\n", fname);
        exit(1);
    }
    if (mm_read_banner(f, &matcode) != 0) {
        fprintf(stderr, "Error: unable to read the Matrix Market banner.\n");
        exit(1);
    }
    if (mm_read_mtx_crd_size(f, &M, &N, &nz) != 0) {
        fprintf(stderr, "Error: unable to read matrix dimensions.\n");
        exit(1);
    }

    printf("\nDimensions: %d x %d\n", M, N);
    printf("Number of non-zeros: %d\n", nz);
    printf("Matrix type: %s\n", mm_typecode_to_str(matcode));

    /* Determine whether input is symmetric */
    int symmetric = mm_is_symmetric(matcode);

    /* Worst-case temp allocation: if symmetric, we may need up to 2*nz entries */
    int temp_alloc = symmetric ? (2 * nz) : nz;

    /* Temporary arrays to store (row,col,val) in 0-based indexing */
    int *I = (int *)malloc(temp_alloc * sizeof(int));
    int *J = (int *)malloc(temp_alloc * sizeof(int));
    double *val = (double *)malloc(temp_alloc * sizeof(double));
    if (!I || !J || !val) {
        fprintf(stderr, "Error: temporary malloc failed\n");
        exit(1);
    }

    /* Read the original nz entries and expand if symmetric */
    int count = 0;
    for (int k = 0; k < nz; k++) {
        int row, col;
        double value;
        if (fscanf(f, "%d %d %lf\n", &row, &col, &value) != 3) {
            fprintf(stderr, "Error reading entry %d\n", k);
            exit(1);
        }
        /* convert to 0-based */
        row--; col--;

        /* store original */
        I[count] = row;
        J[count] = col;
        val[count] = value;
        count++;

        /* if symmetric and off-diagonal, add mirrored entry */
        if (symmetric && row != col) {
            /* ensure we don't exceed allocated temp space (shouldn't happen) */
            if (count >= temp_alloc) {
                /* reallocate larger temp arrays */
                temp_alloc *= 2;
                I = (int *)realloc(I, temp_alloc * sizeof(int));
                J = (int *)realloc(J, temp_alloc * sizeof(int));
                val = (double *)realloc(val, temp_alloc * sizeof(double));
                if (!I || !J || !val) {
                    fprintf(stderr, "Error: temporary realloc failed\n");
                    exit(1);
                }
            }
            I[count] = col;
            J[count] = row;
            val[count] = value;
            count++;
        }
    }
    fclose(f);

    /* Now 'count' is the real number of stored entries after expansion */
    nz = count;

    /* Allocate final CSR arrays (aligned) */
    double *values = (double *)alloc_aligned(nz * sizeof(double), 64);
    int    *col_idx = (int *)alloc_aligned(nz * sizeof(int), 64);
    int    *row_ptr = (int *)calloc(M + 1, sizeof(int));
    if (!values || !col_idx || !row_ptr) {
        fprintf(stderr, "Error: aligned allocation failed\n");
        exit(1);
    }

    /* Count entries per row */
    for (int i = 0; i < nz; i++) {
        int r = I[i];
        if (r < 0 || r >= M) {
            fprintf(stderr, "Error: row index out of range (%d)\n", r);
            exit(1);
        }
        row_ptr[r + 1]++;
    }

    /* Prefix sum to build row_ptr */
    for (int i = 0; i < M; i++) row_ptr[i + 1] += row_ptr[i];

    /* Temporary offsets to place elements */
    int *offset = (int *)calloc(M, sizeof(int));
    if (!offset) { fprintf(stderr, "Error: calloc offset failed\n"); exit(1); }

    /* Fill CSR arrays */
    for (int i = 0; i < nz; i++) {
        int r = I[i];
        int dest = row_ptr[r] + offset[r];
        values[dest] = val[i];
        col_idx[dest] = J[i];
        offset[r]++;
    }

    /* Free temporaries */
    free(I);
    free(J);
    free(val);
    free(offset);

    /* Output results */
    *M_out = M;
    *N_out = N;
    *nz_out = nz;
    *values_out = values;
    *col_idx_out = col_idx;
    *row_ptr_out = row_ptr;
}

/* ---------- Read vector from Matrix Market file (aligned) ---------- */
/* Reads a column vector stored in Matrix Market (.mtx) format and 
   returns it as an aligned array suitable for SIMD/vectorized operations. */
double *read_vector_mtx_aligned(const char *fname, int *length_out){
    FILE *f;              // File pointer to read the vector file
    MM_typecode matcode;   // Matrix Market type code
    int M, N;              // Vector dimensions (M rows, N columns)

    // Open the Matrix Market vector file
    if ((f = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "Error: unable to open vector file %s\n", fname);
        exit(1);
    }

    // Read the Matrix Market banner (check file type and format)
    if (mm_read_banner(f, &matcode) != 0) {
        fprintf(stderr, "Error: could not process Matrix Market banner.\n");
        exit(1);
    }

    // Read vector size
    if (mm_read_mtx_array_size(f, &M, &N) != 0) {
        fprintf(stderr, "Error: could not read vector dimensions.\n");
        exit(1);
    }

    // Ensure it's a column vector (N must be 1)
    if (N != 1) {
        fprintf(stderr, "Error: file %s is not a column vector (cols=%d)\n", fname, N);
        exit(1);
    }

    // Allocate aligned memory for the vector
    double *vec = (double *)alloc_aligned(M * sizeof(double), 64);

    // Read each element from the file into the aligned array
    for (int i = 0; i < M; i++) {
        if (fscanf(f, "%lf", &vec[i]) != 1) {
            fprintf(stderr, "Error: reading value %d from %s\n", i, fname);
            exit(1);
        }
    }

    fclose(f);         // Close the file after reading
    *length_out = M;   // Return the vector length
    return vec;        // Return the aligned vector array
}

/* ---------- Optimized CSR × Vector multiplication ---------- */
/* Performs y = A * x, where A is in CSR (Compressed Sparse Row) format.
   Uses loop unrolling for better instruction-level parallelism (ILP) and cache locality. */
double csr_vector_multiply(int M,
                           const double *restrict values,     // Non-zero values of the matrix
                           const int *restrict col_idx,       // Column indices for each value
                           const int *restrict row_ptr,       // Row pointer array (CSR)
                           const double *restrict x,          // Input vector
                           double *restrict y,                // Output vector
                           int thread_count, int chunk_size){           
                             
    // Loop over each row of the matrix
    double start = omp_get_wtime();
    #pragma omp parallel for num_threads(thread_count) schedule(guided, chunk_size)
    for (int i = 0; i < M; i++) {
        double sum = 0.0;           // Accumulate the dot product for the row
        int start = row_ptr[i];     // Index of first non-zero in this row
        int end   = row_ptr[i + 1]; // Index of last non-zero + 1 in this row

        int j;
        // Loop unrolling: process 4 elements per iteration to improve ILP
        for (j = start; j <= end - 4; j += 4) {
            sum += values[j]     * x[col_idx[j]] +
                   values[j + 1] * x[col_idx[j + 1]] +
                   values[j + 2] * x[col_idx[j + 2]] +
                   values[j + 3] * x[col_idx[j + 3]];
        }

        // Process any remaining elements (less than 4)
        for (; j < end; j++) {
            sum += values[j] * x[col_idx[j]];
        }

        y[i] = sum; // Store the result for row i
    }
    double end = omp_get_wtime();
    return end - start;
}

/* ---------- MAIN ---------- */
/* Main program for reading a sparse matrix in CSR format and a vector,
   performing CSR × vector multiplication, and measuring elapsed time. */
int main(int argc, char *argv[]) {

    // Check command-line arguments
    if(argc < 5){
        printf("Usage: %s matrix.mtx vector.txt #threads #chunk_size\n", argv[0]);
        return 1;
    }

    int M, N, nz;               // Matrix dimensions and number of non-zeros
    double *values;             // CSR non-zero values
    int *col_idx, *row_ptr;     // CSR column indices and row pointers
    double *x, *y;              // Input vector and output vector
    double elapsed;             // Timing variables
    int thread_count = strtol(argv[3], NULL, 10);
    int chunk_size = strtol(argv[4], NULL, 10);

    // Read sparse matrix in CSR format
    read_matrix_csr(argv[1], &M, &N, &nz, &values, &col_idx, &row_ptr);

    // Allocate aligned memory for input and output vectors
    x = (double *)alloc_aligned(N * sizeof(double), 64); // Input vector
    y = (double *)alloc_aligned(M * sizeof(double), 64); // Output vector
    memset(y, 0, M * sizeof(double));                     // Initialize output to zero

    // Read input vector from file
    int vec_len;
    double *vec_file = read_vector_mtx_aligned(argv[2], &vec_len);
    if (vec_len != N) {
        fprintf(stderr, "Error: vector length (%d) does not match matrix columns (%d)\n", vec_len, N);
        exit(1);
    }
    memcpy(x, vec_file, N * sizeof(double)); // Copy data into aligned input vector

#if defined(_WIN32)
    _aligned_free(vec_file); // Free temporary vector memory on Windows
#else
    free(vec_file);          // Free temporary vector memory on Linux
#endif

    // Measure the time of CSR × vector multiplication
    elapsed = csr_vector_multiply(M, values, col_idx, row_ptr, x, y, thread_count, chunk_size);

    printf("\nElapsed time = %e seconds with %d threads and chunksize %d\n\n", elapsed, thread_count, chunk_size);

    // Free all allocated memory
#if defined(_WIN32)
    _aligned_free(values);
    _aligned_free(col_idx);
    _aligned_free(row_ptr);
    _aligned_free(x);
    _aligned_free(y);
#else
    free(values);
    free(col_idx);
    free(row_ptr);
    free(x);
    free(y);
#endif

    return 0;
}