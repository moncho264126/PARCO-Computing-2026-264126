#!/bin/bash

SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

SOURCE="$SOURCE_DIR/mpi_spmv_concurrent.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="mpi_spmv_concurrent"

MATRIX="$MATRIX_DIR/SSM.mtx"
VECTOR="$MATRIX_DIR/SSV.mtx"

PROCS_LIST=(1 2 4 8 16 32 64 128)
N_RUNS=10

mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling MPI code..."
mpicc -O3 -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

for NP in "${PROCS_LIST[@]}"; do

    RESULT_FILE="$RESULTS_DIR/strong_concurrent_np_${NP}.txt"
    echo "Running strong concurrent scaling with NP=$NP"

    echo "Strong concurrent scaling NP=$NP" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((i=1; i<=N_RUNS; i++)); do
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")
        
        # PARSING EXACTO
        LINE=$(echo "$OUT" | grep "SpMV Total:")
        
        T_TOTAL=$(echo "$LINE" | awk '{print $3}')
        T_COMM=$(echo "$LINE" | awk '{print $7}')
        T_COMP=$(echo "$LINE" | awk '{print $11}')

        echo "Run $i: $T_TOTAL | $T_COMM | $T_COMP"
        echo "$i | $T_TOTAL | $T_COMM | $T_COMP" >> "$RESULT_FILE"
    done

    echo ""
done

echo "All strong scaling experiments completed."