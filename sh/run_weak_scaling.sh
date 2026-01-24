#!/bin/bash

# ==============================
# Paths and names
# ==============================
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

SOURCE="$SOURCE_DIR/mpi_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="mpi_spmv"

# ==============================
# Weak scaling configuration
# ==============================
PROCS_LIST=(1 2 4 8 16 32 64 128)

# Matching matrices and vectors
MATRICES=(WS1.mtx WS2.mtx WS3.mtx WS4.mtx WS5.mtx WS6.mtx WS7.mtx WS8.mtx)
VECTORS=(WSV1.mtx WSV2.mtx WSV3.mtx WSV4.mtx WSV5.mtx WSV6.mtx WSV7.mtx WSV8.mtx)

N_RUNS=10

# ==============================
# Create directories
# ==============================
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ==============================
# Compile (Usando -O3 para mejor rendimiento como hablamos)
# ==============================
echo "Compiling MPI code..."
mpicc -O3 -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

# ==============================
# Run weak scaling experiments
# ==============================
for i in "${!PROCS_LIST[@]}"; do

    NP=${PROCS_LIST[$i]}
    MATRIX="$MATRIX_DIR/${MATRICES[$i]}"
    VECTOR="$MATRIX_DIR/${VECTORS[$i]}"

    RESULT_FILE="$RESULTS_DIR/weak_np_${NP}.txt"

    echo "Running weak scaling with NP=$NP"
    echo "Matrix: ${MATRICES[$i]}, Vector: ${VECTORS[$i]}"

    # Encabezado para el archivo de texto
    echo "Weak scaling NP=$NP - Matrix: ${MATRICES[$i]}" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((run=1; run<=N_RUNS; run++)); do
        echo "Run $run / $N_RUNS"

        # Ejecutar y capturar toda la salida
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")

        # Extraer los 3 valores usando grep y awk
        # Buscamos el número que está al final de cada línea (el cuarto o segundo campo según la línea)
        T_TOTAL=$(echo "$OUT" | grep "SpMV Total Time" | awk '{print $4}')
        T_COMM=$(echo "$OUT" | grep "Communication:" | awk '{print $2}')
        T_COMP=$(echo "$OUT" | grep "Computation:" | awk '{print $2}')

        # Guardar en el archivo de resultados en una sola línea para que sea fácil de pasar a Excel/Matplotlib
        echo "$run | $T_TOTAL | $T_COMM | $T_COMP" | tee -a "$RESULT_FILE"
    done

    echo ""
done

echo "All weak scaling experiments completed."