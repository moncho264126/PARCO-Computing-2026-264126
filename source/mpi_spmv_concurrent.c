#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mmio.h"

// Función auxiliar para que cada proceso salte el encabezado del archivo .mtx
void skip_mm_header(FILE *f) {
    MM_typecode matcode;
    int M, N, nz;
    rewind(f);
    mm_read_banner(f, &matcode);
    mm_read_mtx_crd_size(f, &M, &N, &nz);
}

void read_matrix_header(const char *fname, int *M, int *N, int *nz, int *symmetric) {
    FILE *f = fopen(fname, "r");
    MM_typecode matcode;
    if (!f) { fprintf(stderr, "Error opening %s\n", fname); MPI_Abort(MPI_COMM_WORLD, 1); }
    mm_read_banner(f, &matcode);
    mm_read_mtx_crd_size(f, M, N, nz);
    *symmetric = mm_is_symmetric(matcode);
    fclose(f);
}

double *read_vector_mtx(const char *fname, int *n_out) {
    FILE *f = fopen(fname, "r");
    MM_typecode matcode;
    int M, N;
    if (!f) { perror("fopen vector"); MPI_Abort(MPI_COMM_WORLD, 1); }
    mm_read_banner(f, &matcode);
    mm_read_mtx_array_size(f, &M, &N);
    double *x = malloc(M * sizeof(double));
    for (int i = 0; i < M; i++) fscanf(f, "%lf", &x[i]);
    fclose(f);
    *n_out = M;
    return x;
}

void csr_spmv(int nrows, double *values, int *col_idx, int *row_ptr, double *x, double *y) {
    for (int i = 0; i < nrows; i++) {
        double sum = 0.0;
        for (int j = row_ptr[i]; j < row_ptr[i + 1]; j++)
            sum += values[j] * x[col_idx[j]];
        y[i] = sum;
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) printf("Usage: %s matrix.mtx vector.mtx\n", argv[0]);
        MPI_Finalize(); return 0;
    }

    int M, N, nz, symmetric;
    if (rank == 0) read_matrix_header(argv[1], &M, &N, &nz, &symmetric);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nz, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&symmetric, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 2. PROPIEDAD DE FILAS (Round-Robin)
    int local_rows = 0;
    for (int i = 0; i < M; i++) if (i % size == rank) local_rows++;

    int *row_ptr = calloc(local_rows + 1, sizeof(int));

    // 3. PRIMERA PASADA: CONTEO NNZ
    FILE *f = fopen(argv[1], "r");
    skip_mm_header(f); 
    int r, c; double v;
    while (fscanf(f, "%d %d %lf", &r, &c, &v) == 3) {
        r--; c--;
        if (r % size == rank) row_ptr[(r / size) + 1]++;
        if (symmetric && r != c && c % size == rank) row_ptr[(c / size) + 1]++;
    }

    for (int i = 0; i < local_rows; i++) row_ptr[i + 1] += row_ptr[i];
    int my_nnz = row_ptr[local_rows];

    double *values = malloc(my_nnz * sizeof(double));
    int *col_idx = malloc(my_nnz * sizeof(int));
    int *offset = calloc(local_rows, sizeof(int));

    // 4. SEGUNDA PASADA: LLENAR CSR
    skip_mm_header(f);
    while (fscanf(f, "%d %d %lf", &r, &c, &v) == 3) {
        r--; c--;
        if (r % size == rank) {
            int lr = r / size;
            int pos = row_ptr[lr] + offset[lr]++;
            values[pos] = v; col_idx[pos] = c;
        }
        if (symmetric && r != c && c % size == rank) {
            int lr = c / size;
            int pos = row_ptr[lr] + offset[lr]++;
            values[pos] = v; col_idx[pos] = r;
        }
    }
    fclose(f);
    free(offset);

    // 5. VECTOR X + MEDICIÓN
    double *x = (rank == 0) ? read_vector_mtx(argv[2], &M) : malloc(N * sizeof(double));
    
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double t_comm_end = MPI_Wtime();

    double *y = calloc(local_rows, sizeof(double));
    csr_spmv(local_rows, values, col_idx, row_ptr, x, y);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_end = MPI_Wtime();

    if (rank == 0) {
        printf("\nSpMV Total: %e s | Comm: %e s | Comp: %e s\n", 
                t_end - t_start, t_comm_end - t_start, t_end - t_comm_end);
    }

    free(values); free(col_idx); free(row_ptr); free(x); free(y);
    MPI_Finalize();
    return 0;
}