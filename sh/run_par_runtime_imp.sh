#!/bin/bash

# Variables
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
SOURCE="$SOURCE_DIR/par_runtime_imp_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="par_runtime_imp_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

BUILD_DIR="build"
RESULTS_DIR="results"

# Configuración para probar únicamente schedule GUIDED
SCHEDULE_TYPE="guided"
THREADS=(1 2 4 8 16 32 64)
CHUNKSIZES=(1 100 1000)

mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling..."
gcc -std=c99 -O3 -march=native -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO" -lm
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi
echo "Compilation successful."

SELECTED_INDICES=(0 1 2)
# Loop principal
for idx in "${SELECTED_INDICES[@]}"; do
    MATRIX_FILE="${MATRICES[$idx]}"
    VECTOR_FILE="${VECTORS[$idx]}"

    MATRIX="$MATRIX_DIR/$MATRIX_FILE"
    VECTOR="$MATRIX_DIR/$VECTOR_FILE"
    PCT=$(basename "$MATRIX_FILE" .mtx)

    for THREAD in "${THREADS[@]}"; do
        for CHUNK in "${CHUNKSIZES[@]}"; do

            # Configuramos variables de entorno para OpenMP
            export OMP_NUM_THREADS="$THREAD"
            export OMP_SCHEDULE="${SCHEDULE_TYPE},${CHUNK}"

            OUTFILE="$RESULTS_DIR/par_runtime_imp_${THREAD}_${CHUNK}_${PCT}.txt"
            echo "Running $SCHEDULE_TYPE | matrix=$PCT | vector=$VECTOR_FILE | threads=$THREAD | chunk=$CHUNK"

            echo "Schedule = $SCHEDULE_TYPE | Threads = $THREAD | Chunk = $CHUNK" > "$OUTFILE"

            for i in {1..10}; do
                OUT=$("$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")
                echo "$OUT" | grep "Elapsed time" | tee -a "$OUTFILE"
            done

            echo "Saved → $OUTFILE"
            echo ""
        done
    done
done

echo "ALL RUNS COMPLETED"