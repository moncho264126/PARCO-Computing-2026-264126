#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <omp.h> // <--- NUEVO: Librería OpenMP

// Definir un bloque seguro de lectura (< 2GB) para evitar overflow de 'int'
#define MAX_CHUNK_SIZE 1073741824 // 1 GB

typedef struct {
    int r, c;
    double v;
} Triplet;

int compare_triplets(const void *a, const void *b) {
    Triplet *t1 = (Triplet *)a;
    Triplet *t2 = (Triplet *)b;
    if (t1->r != t2->r) return t1->r - t2->r;
    return t1->c - t2->c;
}

// Función robusta para leer Header en Rank 0 (ignora comentarios infinitos)
void read_header_rank0(const char *filename, int *M, int *N, long long *nz, int *symmetric, MPI_Offset *data_start_offset) {
    FILE *f = fopen(filename, "rb"); 
    if (!f) {
        fprintf(stderr, "Error: No se pudo abrir %s en Rank 0\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    char line[4096]; 
    *symmetric = 0;
    int header_found = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "symmetric") != NULL) *symmetric = 1;
        if (line[0] == '%') continue;

        // Usamos long long para nz por si acaso supera 2 mil millones
        int n_items = sscanf(line, "%d %d %lld", M, N, nz);
        if (n_items == 3) {
            *data_start_offset = (MPI_Offset)ftell(f);
            header_found = 1;
            break; 
        }
    }
    fclose(f);

    if (!header_found) {
        fprintf(stderr, "Error: No se encontró la cabecera en %s\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

// Alinear offset al inicio de una línea
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

// --- MODIFICACIÓN: Paralelización con OpenMP ---
void csr_spmv(int nrows, double *values, int *col_idx, int *row_ptr, double *x, double *y) {
    // #pragma omp parallel for: Reparte las iteraciones del bucle 'i' entre los hilos.
    // schedule(dynamic, 64): Balanceo de carga para matrices irregulares.
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < nrows; i++) {
        double sum = 0.0;
        for (int j = row_ptr[i]; j < row_ptr[i + 1]; j++) {
            sum += values[j] * x[col_idx[j]];
        }
        y[i] = sum;
    }
}

int main(int argc, char **argv) {
    // --- MODIFICACIÓN: Init thread-safe ---
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 3) {
        if (rank == 0) printf("Uso: %s matrix.mtx vector.mtx\n", argv[0]);
        MPI_Finalize(); return 0;
    }

    int M, N, symmetric;
    long long nz_global; // Cambiado a long long
    MPI_Offset header_size;

    // --- LECTURA HEADER ---
    if (rank == 0) {
        read_header_rank0(argv[1], &M, &N, &nz_global, &symmetric, &header_size);
        printf("Rank 0: Matriz detectada %d x %d, nz=%lld, Sym=%d. Offset=%lld\n", M, N, nz_global, symmetric, (long long)header_size);
        // Información de hilos
        printf("Rank 0: Ejecutando con %d hilos OpenMP por proceso MPI.\n", omp_get_max_threads());
    }
    
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&nz_global, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&symmetric, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&header_size, 1, MPI_OFFSET, 0, MPI_COMM_WORLD);

    // --- CALCULO DE OFFSETS ---
    MPI_File fh;
    if (MPI_File_open(MPI_COMM_WORLD, argv[1], MPI_MODE_RDONLY, MPI_INFO_NULL, &fh) != MPI_SUCCESS) {
        fprintf(stderr, "Error MPI_File_open\n"); MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Offset file_size;
    MPI_File_get_size(fh, &file_size);
    MPI_Offset data_area = file_size - header_size;
    MPI_Offset chunk = data_area / size;
    MPI_Offset my_start = header_size + rank * chunk;
    MPI_Offset my_end   = (rank == size - 1) ? file_size : header_size + (rank + 1) * chunk;

    if (rank != 0) my_start = find_line_start(fh, my_start, file_size);
    if (rank != size - 1) my_end = find_line_start(fh, my_end, file_size);

    MPI_Offset bytes_to_read_offset = my_end - my_start;
    if (bytes_to_read_offset < 0) bytes_to_read_offset = 0;

    // --- ALOCACIÓN DE BUFFER DE LECTURA ---
    // Chequeo crítico de memoria
    char *buffer = (char*) malloc(bytes_to_read_offset + 1);
    if (!buffer && bytes_to_read_offset > 0) {
        fprintf(stderr, "Rank %d ERROR: Fallo malloc buffer lectura (%lld bytes). RAM insuficiente.\n", rank, (long long)bytes_to_read_offset);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // --- LECTURA POR BLOQUES (FIX > 2GB) ---
    MPI_Offset current_read_pos = my_start;
    MPI_Offset remaining = bytes_to_read_offset;
    char *buf_ptr = buffer;

    while (remaining > 0) {
        int read_chunk = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : (int)remaining;
        MPI_Status status;
        
        int ret = MPI_File_read_at(fh, current_read_pos, buf_ptr, read_chunk, MPI_CHAR, &status);
        
        if (ret != MPI_SUCCESS) {
             fprintf(stderr, "Rank %d Error lectura MPIIO\n", rank); MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        current_read_pos += read_chunk;
        buf_ptr += read_chunk;
        remaining -= read_chunk;
    }
    buffer[bytes_to_read_offset] = '\0'; // Null terminate
    MPI_File_close(&fh);

    // --- PARSING ---
    size_t cap = (bytes_to_read_offset / 20) + 100; 
    Triplet *triplets = malloc(cap * sizeof(Triplet));
    if (!triplets) { fprintf(stderr, "Rank %d OOM triplets\n", rank); MPI_Abort(MPI_COMM_WORLD, 1); }

    size_t count = 0;
    char *curr = buffer;
    char *next_token;

    while (*curr != '\0') {
        while (isspace(*curr)) curr++;
        if (*curr == '\0') break;
        if (*curr == '%') { // Comentario defensivo
            while (*curr != '\n' && *curr != '\0') curr++;
            continue;
        }

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

        if (count >= cap) { 
            cap = cap * 3 / 2 + 100; 
            Triplet *temp = realloc(triplets, cap * sizeof(Triplet));
            if (!temp) { fprintf(stderr, "Rank %d OOM realloc triplets\n", rank); MPI_Abort(MPI_COMM_WORLD, 1); }
            triplets = temp;
        }
        triplets[count++] = (Triplet){r, c, v};

        if (symmetric && r != c) {
            if (count >= cap) { 
                cap = cap * 3 / 2 + 100; 
                Triplet *temp = realloc(triplets, cap * sizeof(Triplet));
                if (!temp) { fprintf(stderr, "Rank %d OOM realloc sym\n", rank); MPI_Abort(MPI_COMM_WORLD, 1); }
                triplets = temp;
            }
            triplets[count++] = (Triplet){c, r, v};
        }
    }
    free(buffer);

    // --- REDISTRIBUCION ---
    int *send_counts = calloc(size, sizeof(int));
    for (size_t i = 0; i < count; i++) send_counts[triplets[i].r % size]++;

    int *recv_counts = malloc(size * sizeof(int));
    MPI_Alltoall(send_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    int *sdispls = malloc(size * sizeof(int));
    int *rdispls = malloc(size * sizeof(int));
    sdispls[0] = 0; rdispls[0] = 0;
    long long total_recv_long = 0; 
    for (int i = 0; i < size; i++) total_recv_long += recv_counts[i];

    // Chequeo de seguridad para buffer de recepción
    if (total_recv_long > INT_MAX) {
        fprintf(stderr, "Rank %d Error: Recibiendo demasiados elementos para int (%lld)\n", rank, total_recv_long);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    int total_recv = (int)total_recv_long;

    for (int i = 1; i < size; i++) {
        sdispls[i] = sdispls[i-1] + send_counts[i-1];
        rdispls[i] = rdispls[i-1] + recv_counts[i-1];
    }

    Triplet *send_buf = malloc(count * sizeof(Triplet));
    if (!send_buf && count > 0) { fprintf(stderr, "Rank %d OOM send_buf\n", rank); MPI_Abort(MPI_COMM_WORLD, 1); }

    int *offsets = calloc(size, sizeof(int));
    for (size_t i = 0; i < count; i++) {
        int target = triplets[i].r % size;
        send_buf[sdispls[target] + offsets[target]++] = triplets[i];
    }
    free(triplets); free(offsets); 

    Triplet *recv_buf = malloc(total_recv * sizeof(Triplet));
    if (!recv_buf && total_recv > 0) { fprintf(stderr, "Rank %d OOM recv_buf\n", rank); MPI_Abort(MPI_COMM_WORLD, 1); }

    MPI_Datatype MPI_TRIPLET;
    MPI_Type_contiguous(sizeof(Triplet), MPI_BYTE, &MPI_TRIPLET);
    MPI_Type_commit(&MPI_TRIPLET);
    MPI_Alltoallv(send_buf, send_counts, sdispls, MPI_TRIPLET, recv_buf, recv_counts, rdispls, MPI_TRIPLET, MPI_COMM_WORLD);
    free(send_buf); free(send_counts); free(recv_counts); free(sdispls); free(rdispls);
    MPI_Type_free(&MPI_TRIPLET);

    // --- CSR LOCAL ---
    qsort(recv_buf, total_recv, sizeof(Triplet), compare_triplets);

    int local_rows_count = 0;
    for (int i = 0; i < M; i++) if (i % size == rank) local_rows_count++;

    int *row_ptr = calloc(local_rows_count + 1, sizeof(int));
    double *values = malloc(total_recv * sizeof(double));
    int *col_idx = malloc(total_recv * sizeof(int));

    if (!row_ptr || (!values && total_recv>0) || (!col_idx && total_recv>0)) {
        fprintf(stderr, "Rank %d OOM CSR struct\n", rank); MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < total_recv; i++) {
        int local_r = recv_buf[i].r / size;
        if (local_r < local_rows_count) row_ptr[local_r + 1]++;
    }
    for (int i = 0; i < local_rows_count; i++) row_ptr[i+1] += row_ptr[i];

    int *curr_pos = calloc(local_rows_count, sizeof(int));
    for (int i = 0; i < total_recv; i++) {
        int local_r = recv_buf[i].r / size;
        if (local_r >= local_rows_count) continue; 
        int p = row_ptr[local_r] + curr_pos[local_r]++;
        values[p] = recv_buf[i].v;
        col_idx[p] = recv_buf[i].c;
    }
    free(curr_pos); free(recv_buf);

    // --- VECTOR X (CRITICAL SECTION FOR RAM) ---
    double *x_global = malloc(N * sizeof(double));
    if (!x_global) {
        fprintf(stderr, "Rank %d FATAL ERROR: No hay RAM para vector X (%ld bytes)\n", rank, N * sizeof(double));
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0) {
        FILE *fv = fopen(argv[2], "r");
        if (fv) {
            char line[256];
            do { fgets(line, 256, fv); } while(line[0] == '%');
            int vm, vn; sscanf(line, "%d %d", &vm, &vn);
            for(int i=0; i<vm; i++) {
                if(fscanf(fv, "%lf", &x_global[i]) != 1) break;
            }
            fclose(fv);
        } else {
             fprintf(stderr, "Error abriendo vector %s. Usando 0.0\n", argv[2]);
             for(int i=0; i<N; i++) x_global[i] = 1.0; // Valor dummy para evitar crash
        }
    }

    double *y = calloc(local_rows_count, sizeof(double));

    // --- TIMING ORIGINAL (INTACTO) ---
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    MPI_Bcast(x_global, N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double t_comm = MPI_Wtime();
    
    // Esta llamada ahora es multihilo internamente
    csr_spmv(local_rows_count, values, col_idx, row_ptr, x_global, y);
    
    MPI_Barrier(MPI_COMM_WORLD);
    double t_end = MPI_Wtime();

    if (rank == 0) {
        printf("SpMV Total: %e s | Comm: %e s | Comp: %e s\n", 
               t_end - t_start, t_comm - t_start, t_end - t_comm);
    }
    
    free(values); free(col_idx); free(row_ptr);
    free(x_global); free(y);

    MPI_Finalize();
    return 0;
}