/********************************************************************************************
 * Description:
 *    High-performance parallel Sparse Matrix-Vector multiplication (SpMV) benchmark
 *    using the Compressed Sparse Row (CSR) format and OpenMP. 
 *
 *    This program:
 *        1. Reads a sparse matrix stored in Matrix Market (.mtx) coordinate format.
 *        2. Converts it into CSR representation, expanding symmetric matrices when needed.
 *        3. Reads an input vector from a Matrix Market file.
 *        4. Performs y = A * x using a CSR kernel with OpenMP parallelization and runtime scheduling.
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
 *        gcc -g -Wall -fopenmp -o par_runtime_spmv par_runtime_spmv.c mmio.c
 *
 * Execution:
 *        ./par_runtime_spmv matrix.mtx vector.mtx
 *
 * Output:
 *        - Elapsed time (seconds)
 *
 * Requirements:
 *        - mmio.c/mmio.h for Matrix Market parsing
 ********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
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

// ------------------ READ VECTOR ------------------
double *read_vector_mtx(const char *fname, int *length_out){
    FILE *f;
    MM_typecode matcode;
    int M, N;
    if(mm_read_banner((f = fopen(fname, "r")), &matcode) != 0){
        fprintf(stderr, "Cannot read vector banner.\n");
        exit(1);
    }
    if(mm_read_mtx_array_size(f, &M, &N) != 0){
        fprintf(stderr, "Cannot read vector size.\n");
        exit(1);
    }

    double *vec = malloc(M * sizeof(double));
    for(int i = 0; i < M; i++)
        fscanf(f, "%lf", &vec[i]);
    fclose(f);

    *length_out = M;
    return vec;
}

// ------------------ SPARSE MATRIX × VECTOR ------------------
double csr_vector_multiply(int M, double *values, int *col_idx, int *row_ptr, double *x, double *y){
    double start = omp_get_wtime();

    #pragma omp parallel for schedule(runtime)
    for(int i = 0; i < M; i++){
        double sum = 0.0;
        for(int j = row_ptr[i]; j < row_ptr[i + 1]; j++)
            sum += values[j] * x[col_idx[j]];
        y[i] = sum;
    }

    return omp_get_wtime() - start;
}

// ------------------ MAIN ------------------
int main(int argc, char *argv[]) {

    if(argc < 3){
        printf("Usage: %s matrix.mtx vector.mtx\n", argv[0]);
        printf("Threads and schedule are read from OMP_NUM_THREADS and OMP_SCHEDULE\n");
        return 1;
    }

    int M, N, nz;
    double *values, *x;
    int *col_idx, *row_ptr;

    read_matrix_csr(argv[1], &M, &N, &nz, &values, &col_idx, &row_ptr);
    x = read_vector_mtx(argv[2], &N);

    double *y = calloc(M, sizeof(double));
    double elapsed = csr_vector_multiply(M, values, col_idx, row_ptr, x, y);

    printf("Elapsed time = %e seconds | threads=%d | OMP_SCHEDULE=%s\n", elapsed, omp_get_max_threads(), getenv("OMP_SCHEDULE"));

    free(values); free(col_idx); free(row_ptr); free(x); free(y);
    return 0;
}