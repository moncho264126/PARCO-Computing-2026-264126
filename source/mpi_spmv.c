// mpicc -o mpi_spmv mpi_spmv.c mmio.c
// mpirun -np 4 ./mpi_spmv matrix.mtx vector.mtx

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mmio.h"

/* =========================================================
   READ MATRIX IN CSR (ONLY RANK 0 USES THIS)
   ========================================================= */
void read_matrix_csr(const char *fname, int *M_out, int *N_out, int *nz_out,
                     double **values_out, int **col_idx_out, int **row_ptr_out)
{
    FILE *f;
    MM_typecode matcode;
    int M, N, nz;

    if ((f = fopen(fname, "r")) == NULL){
        fprintf(stderr, "Cannot open %s\n", fname);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    mm_read_banner(f, &matcode);
    mm_read_mtx_crd_size(f, &M, &N, &nz);

    int symmetric = mm_is_symmetric(matcode);
    int alloc = symmetric ? 2 * nz : nz;

    int *I = malloc(alloc * sizeof(int));
    int *J = malloc(alloc * sizeof(int));
    double *V = malloc(alloc * sizeof(double));

    int count = 0;
    for(int k = 0; k < nz; k++){
        int r, c;
        double v;
        fscanf(f, "%d %d %lf", &r, &c, &v);
        r--; c--;
        I[count] = r;
        J[count] = c;
        V[count] = v;
        count++;
        if(symmetric && r != c){
            I[count] = c;
            J[count] = r;
            V[count] = v;
            count++;
        }
    }
    fclose(f);
    nz = count;

    double *values = malloc(nz * sizeof(double));
    int *col_idx = malloc(nz * sizeof(int));
    int *row_ptr = calloc(M + 1, sizeof(int));

    for(int i = 0; i < nz; i++)
        row_ptr[I[i] + 1]++;

    for(int i = 0; i < M; i++)
        row_ptr[i + 1] += row_ptr[i];

    int *offset = calloc(M, sizeof(int));
    for(int i = 0; i < nz; i++){
        int r = I[i];
        int pos = row_ptr[r] + offset[r];
        values[pos] = V[i];
        col_idx[pos] = J[i];
        offset[r]++;
    }

    free(I); free(J); free(V); free(offset);

    *M_out = M;
    *N_out = N;
    *nz_out = nz;
    *values_out = values;
    *col_idx_out = col_idx;
    *row_ptr_out = row_ptr;
}

/* =========================================================
   READ VECTOR (ONLY RANK 0)
   ========================================================= */
double *read_vector_mtx(const char *fname, int *n_out)
{
    FILE *f;
    MM_typecode matcode;
    int M, N;

    f = fopen(fname, "r");
    mm_read_banner(f, &matcode);
    mm_read_mtx_array_size(f, &M, &N);

    double *x = malloc(M * sizeof(double));
    for(int i = 0; i < M; i++)
        fscanf(f, "%lf", &x[i]);

    fclose(f);
    *n_out = M;
    return x;
}

/* =========================================================
   LOCAL CSR SPMV
   ========================================================= */
void csr_spmv(int nrows, double *values, int *col_idx,
              int *row_ptr, double *x, double *y)
{
    for(int i = 0; i < nrows; i++){
        double sum = 0.0;
        for(int j = row_ptr[i]; j < row_ptr[i+1]; j++)
            sum += values[j] * x[col_idx[j]];
        y[i] = sum;
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(argc < 3){
        if(rank == 0) printf("Usage: %s matrix.mtx vector.mtx\n", argv[0]);
        MPI_Finalize();
        return 0;
    }

    int M, N, nz;
    double *A_val = NULL, *x = NULL;
    int *A_col = NULL, *A_row = NULL;

    // 1. RANK 0 LEE TODO
    if(rank == 0){
        read_matrix_csr(argv[1], &M, &N, &nz, &A_val, &A_col, &A_row);
        int nvec;
        x = read_vector_mtx(argv[2], &nvec);
    }

    // 2. BROADCAST DE DIMENSIONES
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 3. CALCULAR LOCAL_ROWS
    int local_rows = 0;
    for(int i = 0; i < M; i++) {
        if(i % size == rank) local_rows++;
    }
    
    int my_nnz = 0;
    double *my_val = NULL;
    int *my_col = NULL;
    int *my_row_ptr = calloc(local_rows + 1, sizeof(int));

    // --- DISTRIBUCIÓN DE LA MATRIZ (ESTO SE QUEDA IGUAL) ---
    if(rank == 0) {
        int *send_counts = malloc(size * sizeof(int)); 
        for(int r = 0; r < size; r++) send_counts[r] = 0;
        for(int i = 0; i < M; i++) {
            int dest_rank = i % size;
            send_counts[dest_rank] += (A_row[i+1] - A_row[i]);
        }
        my_nnz = send_counts[0];
        for(int r = 1; r < size; r++) {
            MPI_Send(&send_counts[r], 1, MPI_INT, r, 0, MPI_COMM_WORLD);
        }

        my_val = malloc(my_nnz * sizeof(double));
        my_col = malloc(my_nnz * sizeof(int));
        int *current_pos = calloc(size, sizeof(int)); 
        int *current_row = calloc(size, sizeof(int));
        double **p_vals = malloc(size * sizeof(double*));
        int **p_cols = malloc(size * sizeof(int*));
        int **p_rows = malloc(size * sizeof(int*));

        for(int r=0; r<size; r++){
            if(r == 0) {
                p_vals[0] = my_val; p_cols[0] = my_col; p_rows[0] = my_row_ptr;
            } else {
                p_vals[r] = malloc(send_counts[r] * sizeof(double));
                p_cols[r] = malloc(send_counts[r] * sizeof(int));
                int r_rows = 0;
                for(int k=0; k<M; k++) if(k%size == r) r_rows++;
                p_rows[r] = calloc(r_rows + 1, sizeof(int));
            }
        }

        for(int i = 0; i < M; i++) {
            int dest = i % size;
            int len = A_row[i+1] - A_row[i];
            int row_idx = current_row[dest];
            p_rows[dest][row_idx + 1] = p_rows[dest][row_idx] + len;
            current_row[dest]++;
            for(int k = 0; k < len; k++) {
                p_vals[dest][current_pos[dest]] = A_val[A_row[i] + k];
                p_cols[dest][current_pos[dest]] = A_col[A_row[i] + k];
                current_pos[dest]++;
            }
        }

        for(int r = 1; r < size; r++) {
            int r_rows = 0;
            for(int k=0; k<M; k++) if(k%size == r) r_rows++;
            MPI_Send(p_rows[r], r_rows + 1, MPI_INT, r, 1, MPI_COMM_WORLD);
            MPI_Send(p_vals[r], send_counts[r], MPI_DOUBLE, r, 2, MPI_COMM_WORLD);
            MPI_Send(p_cols[r], send_counts[r], MPI_INT, r, 3, MPI_COMM_WORLD);
            free(p_vals[r]); free(p_cols[r]); free(p_rows[r]);
        }
        free(send_counts); free(current_pos); free(current_row);
        free(p_vals); free(p_cols); free(p_rows);

    } else {
        MPI_Recv(&my_nnz, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        my_val = malloc(my_nnz * sizeof(double));
        my_col = malloc(my_nnz * sizeof(int));
        MPI_Recv(my_row_ptr, local_rows + 1, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(my_val, my_nnz, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(my_col, my_nnz, MPI_INT, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    /* =========================================================
       NUEVA MEDICIÓN: COMUNICACIÓN + CÓMPUTO
       ========================================================= */
    
    // Sincronización global antes de empezar
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    // 1. Comunicación (Broadcast del vector x)
    if(rank != 0) x = malloc(N * sizeof(double));
    MPI_Bcast(x, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    double t_comm_end = MPI_Wtime(); // Marca el fin de la comunicación

    // 2. Cómputo (Kernel SpMV local)
    double *y = calloc(local_rows, sizeof(double));
    csr_spmv(local_rows, my_val, my_col, my_row_ptr, x, y);

    // Sincronización final
    MPI_Barrier(MPI_COMM_WORLD);
    double t_end = MPI_Wtime();

    // Cálculos de tiempos
    double total_time = t_end - t_start;
    double comm_time  = t_comm_end - t_start;
    double comp_time  = t_end - t_comm_end;

    if(rank == 0) {
        printf("SpMV Total Time: %e seconds\n", total_time);
        printf("  Communication: %e seconds\n", comm_time);
        printf("  Computation:   %e seconds\n", comp_time);
    }

    // Limpieza final
    free(my_val); free(my_col); free(my_row_ptr); free(x); free(y);
    if(rank == 0) { free(A_val); free(A_col); free(A_row); }

    MPI_Finalize();
    return 0;
}