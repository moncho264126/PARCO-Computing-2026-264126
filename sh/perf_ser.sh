#!/bin/bash

# ------------------------ VARIABLES ------------------------
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
SOURCE="$SOURCE_DIR/ser_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="ser_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

BUILD_DIR="build"
RESULTS_DIR="results_perf/ser"

# ------------------------ CREAR CARPETAS ------------------------
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ------------------------ COMPILAR ------------------------
echo "Compiling..."
gcc -std=c99 -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"
if [ $? -ne 0 ]; then
    echo "Compilation error. Aborted."
    exit 1
fi
echo "Compilation successful."

# ------------------------ LOOP PRINCIPAL ------------------------
for index in "${!MATRICES[@]}"; do

    MATRIX_FILE="${MATRICES[$index]}"
    VECTOR_FILE="${VECTORS[$index]}"

    MATRIX="$MATRIX_DIR/$MATRIX_FILE"
    VECTOR="$MATRIX_DIR/$VECTOR_FILE"

    RESULT_FILE="$RESULTS_DIR/ser_$(basename "$MATRIX_FILE" .mtx).txt"
    echo "Processing matrix $MATRIX with vector $VECTOR"

    echo "Performance results for $MATRIX" > "$RESULT_FILE"
    echo "L1-loads | L1-misses | LLC-loads | LLC-misses" >> "$RESULT_FILE"

    for i in {1..10}; do

        PERF_OUT=$(perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses \
            "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR" 2>&1 1>/dev/null)

        L1_LOADS=$(echo "$PERF_OUT" | grep "L1-dcache-loads" | awk '{print $1}')
        L1_MISSES=$(echo "$PERF_OUT" | grep "L1-dcache-load-misses" | awk '{print $1}')
        LLC_LOADS=$(echo "$PERF_OUT" | grep "LLC-loads" | awk '{print $1}')
        LLC_MISSES=$(echo "$PERF_OUT" | grep "LLC-load-misses" | awk '{print $1}')

        echo "$L1_LOADS | $L1_MISSES | $LLC_LOADS | $LLC_MISSES" >> "$RESULT_FILE"
    done

    echo "Results saved in $RESULT_FILE"
    echo ""
done

echo "ALL MATRICES COMPLETED"