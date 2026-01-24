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

MATRIX="$MATRIX_DIR/SSM.mtx"
VECTOR="$MATRIX_DIR/SSV.mtx"

# MPI process counts for strong scaling
PROCS_LIST=(1 2 4 8 16 32 64 128)

N_RUNS=10

# ==============================
# Create directories
# ==============================
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ==============================
# Compile
# ==============================
echo "Compiling MPI code..."
mpicc -O3 -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

# ==============================
# Run experiments
# ==============================
for NP in "${PROCS_LIST[@]}"; do

    RESULT_FILE="$RESULTS_DIR/strong_np_${NP}.txt"
    echo "Running strong scaling with NP=$NP"

    # Encabezado del archivo
    echo "Strong scaling NP=$NP" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((i=1; i<=N_RUNS; i++)); do
        echo "Run $i / $N_RUNS"

        # Ejecutamos y capturamos la salida completa
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")
        
        # Extraemos cada tiempo usando awk
        T_TOTAL=$(echo "$OUT" | grep "SpMV Total Time" | awk '{print $4}')
        T_COMM=$(echo "$OUT" | grep "Communication:" | awk '{print $2}')
        T_COMP=$(echo "$OUT" | grep "Computation:" | awk '{print $2}')

        # Guardamos los tres valores en una sola línea
        echo "$i | $T_TOTAL | $T_COMM | $T_COMP" | tee -a "$RESULT_FILE"
    done

    echo ""
done

echo "All strong scaling experiments completed."