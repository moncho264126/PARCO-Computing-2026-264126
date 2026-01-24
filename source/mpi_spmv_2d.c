#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include <limits.h>
#include <ctype.h> 

// Chunk de lectura seguro para MPI-IO
#define MAX_CHUNK_SIZE 1073741824 

typedef struct {
    int r, c;
    double v;
} Triplet;

// Estructura para CSR Local
typedef struct {
    double *values;
    int *col_idx;
    int *row_ptr;
    int nrows_local;
    int ncols_local;
    int nnz_local;
} CSRLocal;

// Comparador para ordenar Triplets (Fila, luego Columna)
int compare_triplets(const void *a, const void *b) {
    Triplet *t1 = (Triplet *)a;
    Triplet *t2 = (Triplet *)b;
    if (t1->r != t2->r) return t1->r - t2->r;
    return t1->c - t2->c;
}

// ---------------------------------------------------------------------------
// 1. UTILIDADES DE LECTURA (Manual Parsing robusto)
// ---------------------------------------------------------------------------
void read_header_rank0(const char *filename, int *M, int *N, long long *nz, int *symmetric, MPI_Offset *data_start_offset) {
    FILE *f = fopen(filename, "rb"); 
    if (!f) { fprintf(stderr, "Rank 0: Error abriendo %s\n", filename); MPI_Abort(MPI_COMM_WORLD, 1); }

    char line[4096]; 
    *symmetric = 0;
    int header_found = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "symmetric") != NULL) *symmetric = 1;
        if (line[0] == '%') continue;

        if (sscanf(line, "%d %d %lld", M, N, nz) == 3) {
            *data_start_offset = (MPI_Offset)ftell(f);
            header_found = 1;
            break; 
        }
    }
    fclose(f);
    if (!header_found) { fprintf(stderr, "Error header no encontrado\n"); MPI_Abort(MPI_COMM_WORLD, 1); }
}

MPI_Offset find_line_start(MPI_File fh, MPI_Offset offset, MPI_Offset file_size) {
    if (offset == 0) return 0;
    if (offset >= file_size) return file_size;
    char c;
    MPI_Status status;
    MPI_File_read_at(fh, offset - 1, &c, 1, MPI_CHAR, &status);
    if (c == '\n') return offset;
    MPI_Offset current = offset;
    while (current < file_size) {
        MPI_File_read_at(fh, current, &c, 1, MPI_CHAR, &status);
        current++;
        if (c == '\n') return current;
    }
    return file_size;
}

// ---------------------------------------------------------------------------
// 2. LÓGICA 2D (CHECKERBOARD)
// ---------------------------------------------------------------------------

void get_block_bounds(int N_total, int n_blocks, int coord, int *start, int *end) {
    int base = N_total / n_blocks;
    int rem = N_total % n_blocks;
    if (coord < rem) {
        *start = coord * (base + 1);
        *end = *start + base + 1;
    } else {
        *start = coord * base + rem;
        *end = *start + base;
    }
}

int get_owner_rank(int r, int c, int dims[2], int M, int N, MPI_Comm comm_cart) {
    int row_owner = -1;
    int col_owner = -1;

    for (int i = 0; i < dims[0]; i++) {
        int s, e;
        get_block_bounds(M, dims[0], i, &s, &e);
        if (r >= s && r < e) { row_owner = i; break; }
    }
    
    for (int i = 0; i < dims[1]; i++) {
        int s, e;
        get_block_bounds(N, dims[1], i, &s, &e);
        if (c >= s && c < e) { col_owner = i; break; }
    }

    int coords[2] = {row_owner, col_owner};
    int rank;
    MPI_Cart_rank(comm_cart, coords, &rank);
    return rank;
}

// ---------------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) printf("Uso: %s matrix.mtx vector.mtx\n", argv[0]);
        MPI_Finalize(); return 0;
    }

    // --- 1. SETUP TOPOLOGÍA ---
    int dims[2] = {0, 0};
    MPI_Dims_create(size, 2, dims); 
    int periods[2] = {0, 0};
    MPI_Comm comm_cart;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &comm_cart);
    
    int my_coords[2];
    MPI_Cart_coords(comm_cart, rank, 2, my_coords);
    
    MPI_Comm comm_row, comm_col;
    MPI_Comm_split(comm_cart, my_coords[0], my_coords[1], &comm_row); 
    MPI_Comm_split(comm_cart, my_coords[1], my_coords[0], &comm_col); 

    int M, N, symmetric;
    long long nz_global;
    MPI_Offset header_size;

    if (rank == 0) {
        read_header_rank0(argv[1], &M, &N, &nz_global, &symmetric, &header_size);
    }
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nz_global, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&symmetric, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&header_size, 1, MPI_OFFSET, 0, MPI_COMM_WORLD);

    int r_start, r_end, c_start, c_end;
    get_block_bounds(M, dims[0], my_coords[0], &r_start, &r_end);
    get_block_bounds(N, dims[1], my_coords[1], &c_start, &c_end);
    
    int my_rows = r_end - r_start;
    int my_cols = c_end - c_start;
    // Protección: asegurar que no sean negativos (aunque no deberían)
    if (my_rows < 0) my_rows = 0;
    if (my_cols < 0) my_cols = 0;

    // --- 2. LECTURA MPI-IO ---
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, argv[1], MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    
    MPI_Offset file_size;
    MPI_File_get_size(fh, &file_size);
    MPI_Offset data_area = file_size - header_size;
    MPI_Offset chunk = data_area / size;
    MPI_Offset my_fstart = header_size + rank * chunk;
    MPI_Offset my_fend = (rank == size - 1) ? file_size : header_size + (rank + 1) * chunk;

    if (rank != 0) my_fstart = find_line_start(fh, my_fstart, file_size);
    if (rank != size - 1) my_fend = find_line_start(fh, my_fend, file_size);
    
    MPI_Offset bytes_to_read = my_fend - my_fstart;
    if (bytes_to_read < 0) bytes_to_read = 0;
    
    // CAST EXPLÍCITO A size_t AQUÍ
    char *buffer = malloc((size_t)bytes_to_read + 1);
    
    MPI_Offset curr_read = my_fstart;
    char *buf_ptr = buffer;
    MPI_Offset remaining = bytes_to_read;
    while(remaining > 0) {
        int read_size = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : (int)remaining;
        MPI_Status st;
        MPI_File_read_at(fh, curr_read, buf_ptr, read_size, MPI_CHAR, &st);
        curr_read += read_size;
        buf_ptr += read_size;
        remaining -= read_size;
    }
    buffer[bytes_to_read] = '\0';
    MPI_File_close(&fh);

    size_t cap = (bytes_to_read / 20) + 100;
    Triplet *triplets = malloc(cap * sizeof(Triplet));
    size_t count = 0;
    
    char *curr = buffer;
    char *next_token;
    while (*curr != '\0') {
        while (isspace(*curr)) curr++; 
        if (*curr == '\0') break;
        if (*curr == '%') { while (*curr != '\n' && *curr != '\0') curr++; continue; }

        long r_raw = strtol(curr, &next_token, 10);
        if (curr == next_token) { curr++; continue; }
        curr = next_token;
        long c_raw = strtol(curr, &next_token, 10);
        curr = next_token;
        double v = strtod(curr, &next_token);
        curr = next_token;

        int r = (int)r_raw - 1;
        int c = (int)c_raw - 1;
        if (r < 0 || r >= M || c < 0 || c >= N) continue;

        if (count >= cap) { cap *= 1.5; triplets = realloc(triplets, cap * sizeof(Triplet)); }
        triplets[count++] = (Triplet){r, c, v};

        if (symmetric && r != c) {
            if (count >= cap) { cap *= 1.5; triplets = realloc(triplets, cap * sizeof(Triplet)); }
            triplets[count++] = (Triplet){c, r, v};
        }
    }
    free(buffer);

    // --- 3. REDISTRIBUCIÓN (ALLTOALL) ---
    int *send_counts = calloc(size, sizeof(int));
    int *dest_ranks = malloc(count * sizeof(int));

    for (size_t i = 0; i < count; i++) {
        int owner = get_owner_rank(triplets[i].r, triplets[i].c, dims, M, N, comm_cart);
        dest_ranks[i] = owner;
        send_counts[owner]++;
    }

    int *recv_counts = malloc(size * sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    int *sdispls = malloc(size * sizeof(int));
    int *rdispls = malloc(size * sizeof(int));
    long long total_recv_long = 0;
    sdispls[0] = 0; rdispls[0] = 0;
    
    for (int i = 0; i < size; i++) total_recv_long += recv_counts[i];
    for (int i = 1; i < size; i++) {
        sdispls[i] = sdispls[i-1] + send_counts[i-1];
        rdispls[i] = rdispls[i-1] + recv_counts[i-1];
    }

    Triplet *send_buf = malloc(count * sizeof(Triplet));
    int *offsets = calloc(size, sizeof(int));
    for (size_t i = 0; i < count; i++) {
        int target = dest_ranks[i];
        send_buf[sdispls[target] + offsets[target]++] = triplets[i];
    }
    free(triplets); free(dest_ranks); free(offsets);

    // CAST EXPLÍCITO A size_t
    Triplet *recv_buf = malloc((size_t)total_recv_long * sizeof(Triplet));
    
    MPI_Datatype MPI_TRIPLET;
    MPI_Type_contiguous(sizeof(Triplet), MPI_BYTE, &MPI_TRIPLET);
    MPI_Type_commit(&MPI_TRIPLET);
    
    MPI_Alltoallv(send_buf, send_counts, sdispls, MPI_TRIPLET, 
                  recv_buf, recv_counts, rdispls, MPI_TRIPLET, MPI_COMM_WORLD);
    
    free(send_buf); free(send_counts); free(recv_counts); free(sdispls); free(rdispls);
    MPI_Type_free(&MPI_TRIPLET);

    // --- 4. CSR LOCAL ---
    qsort(recv_buf, (size_t)total_recv_long, sizeof(Triplet), compare_triplets);

    CSRLocal A;
    A.nrows_local = my_rows;
    A.ncols_local = my_cols;
    A.nnz_local = (int)total_recv_long; // Cuidado si supera 2e9, pero es CSRLocal

    // ERROR ANTERIOR AQUÍ: CAST EXPLÍCITO A size_t
    A.row_ptr = calloc((size_t)A.nrows_local + 1, sizeof(int));
    A.values = malloc((size_t)A.nnz_local * sizeof(double));
    A.col_idx = malloc((size_t)A.nnz_local * sizeof(int));

    for (int i = 0; i < A.nnz_local; i++) {
        int local_r = recv_buf[i].r - r_start;
        if (local_r >= 0 && local_r < A.nrows_local)
            A.row_ptr[local_r + 1]++;
    }
    for (int i = 0; i < A.nrows_local; i++) A.row_ptr[i+1] += A.row_ptr[i];

    // --- ESTE ES EL ARREGLO DEL ERROR CRÍTICO ---
    // Usamos (size_t)A.nrows_local para asegurar al compilador que no es negativo
    int *curr_pos = NULL;
    if (A.nrows_local > 0) {
        curr_pos = calloc((size_t)A.nrows_local, sizeof(int));
    }
    // ---------------------------------------------

    for (int i = 0; i < A.nnz_local; i++) {
        int local_r = recv_buf[i].r - r_start;
        if (local_r >= 0 && local_r < A.nrows_local) {
            // Protección adicional por si curr_pos es null (no debería si nrows > 0)
            if (curr_pos) {
                int p = A.row_ptr[local_r] + curr_pos[local_r]++;
                A.values[p] = recv_buf[i].v;
                A.col_idx[p] = recv_buf[i].c - c_start; 
            }
        }
    }
    if (curr_pos) free(curr_pos); 
    free(recv_buf);

    // --- 5. SETUP VECTOR X ---
    double *x_local_block = malloc((size_t)my_cols * sizeof(double));
    double *x_temp_recv = NULL; 
    if (my_coords[0] == 0) x_temp_recv = malloc((size_t)my_cols * sizeof(double));

    if (rank == 0) {
        double *x_full = malloc((size_t)N * sizeof(double));
        
        FILE *fv = fopen(argv[2], "r");
        if (fv) {
            char line[256];
            do { fgets(line, 256, fv); } while(line[0] == '%');
            int vm, vn; sscanf(line, "%d %d", &vm, &vn);
            for(int i=0; i<vm; i++) fscanf(fv, "%lf", &x_full[i]);
            fclose(fv);
        } else {
             for(int i=0; i<N; i++) x_full[i] = 1.0;
        }

        int s0, e0;
        get_block_bounds(N, dims[1], 0, &s0, &e0);
        for(int k=0; k < (e0-s0); k++) x_temp_recv[k] = x_full[s0+k];

        for (int c = 1; c < dims[1]; c++) {
            int cs, ce;
            get_block_bounds(N, dims[1], c, &cs, &ce);
            int target_rank;
            int coords_target[2] = {0, c}; 
            MPI_Cart_rank(comm_cart, coords_target, &target_rank);
            MPI_Send(&x_full[cs], ce - cs, MPI_DOUBLE, target_rank, 99, MPI_COMM_WORLD);
        }
        free(x_full);
    } else if (my_coords[0] == 0) {
        MPI_Recv(x_temp_recv, my_cols, MPI_DOUBLE, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // --- 6. CÁLCULO SpMV (MEDICIÓN DE TIEMPO) ---
    double *y_local_partial = calloc((size_t)my_rows, sizeof(double));
    double *y_local_final = calloc((size_t)my_rows, sizeof(double));

    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime(); 

    // Fase 1: Broadcast Vector X (Comm)
    if (my_coords[0] == 0) {
        memcpy(x_local_block, x_temp_recv, (size_t)my_cols * sizeof(double));
        free(x_temp_recv);
    }
    MPI_Bcast(x_local_block, my_cols, MPI_DOUBLE, 0, comm_col);

    double t_comm_end = MPI_Wtime(); 

    // Fase 2: SpMV Local (Comp)
    for (int i = 0; i < A.nrows_local; i++) {
        double sum = 0.0;
        for (int j = A.row_ptr[i]; j < A.row_ptr[i+1]; j++) {
            sum += A.values[j] * x_local_block[A.col_idx[j]];
        }
        y_local_partial[i] = sum;
    }

    // Fase 3: Reduce Y (Comp/Comm)
    MPI_Reduce(y_local_partial, y_local_final, my_rows, MPI_DOUBLE, MPI_SUM, 0, comm_row);

    double t_end = MPI_Wtime(); 

    // --- REPORTING ---
    if (rank == 0) {
        printf("\nSpMV Total: %e s | Comm: %e s | Comp: %e s\n", 
               t_end - t_start, t_comm_end - t_start, t_end - t_comm_end);
    }

    free(A.values); free(A.col_idx); free(A.row_ptr);
    free(x_local_block); free(y_local_partial); free(y_local_final);

    MPI_Comm_free(&comm_row);
    MPI_Comm_free(&comm_col);
    MPI_Comm_free(&comm_cart);
    MPI_Finalize();
    return 0;
}