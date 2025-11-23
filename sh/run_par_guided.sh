#!/bin/bash

# Variables
SOURCE_DIR="./source"           # Directorio del código fuente
MATRIX_DIR="./matrix"           # Directorio de las matrices .mtx
SOURCE="$SOURCE_DIR/par_guided_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="par_guided_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

BUILD_DIR="build"
RESULTS_DIR="results/par_guided"

# Threads y chunksizes a probar
THREADS=(1 2 4 8 16 32 64)
CHUNKSIZES=(1 100 1000)

# Crear carpetas si no existen
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling..."
gcc -std=c99 -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

# Verificar compilación
if [ $? -ne 0 ]; then
    echo "Compilation error. Aborted."
    exit 1
fi
echo "Compilation successful."

SELECTED_INDICES=(0 1 2 3 4)
# Loop principal
for idx in "${SELECTED_INDICES[@]}"; do
    MATRIX_FILE="${MATRICES[$idx]}"
    VECTOR_FILE="${VECTORS[$idx]}"

    MATRIX="$MATRIX_DIR/$MATRIX_FILE"
    VECTOR="$MATRIX_DIR/$VECTOR_FILE"

    PCT=$(basename "$MATRIX_FILE" .mtx)

    for THREAD in "${THREADS[@]}"; do
        for CHUNK in "${CHUNKSIZES[@]}"; do

            RESULT_FILE="$RESULTS_DIR/par_guided_${THREAD}_${CHUNK}_${PCT}.txt"
            echo "Processing matrix $MATRIX | vector=$VECTOR | threads=$THREAD | chunksize=$CHUNK"

            echo "Results for $MATRIX | threads=$THREAD | chunksize=$CHUNK" > "$RESULT_FILE"

            for i in {1..10}; do
                OUT=$("$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR" "$THREAD" "$CHUNK")
                TIME=$(echo "$OUT" | grep "Elapsed time")
                echo "$TIME" | tee -a "$RESULT_FILE"
            done

            echo "Results saved in $RESULT_FILE"
            echo ""
        done
    done
done

echo "ALL MATRICES COMPLETED"