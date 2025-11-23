#!/bin/bash

# Variables
SOURCE_DIR="./source"           # Directory where the C source code is
MATRIX_DIR="./matrix"           # Directory where the .mtx matrices are
SOURCE="$SOURCE_DIR/par_auto_imp_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="par_auto_imp_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

THREADS_LIST=(1 2 4 8 16 32 64)    # Number of threads to test

BUILD_DIR="build"
RESULTS_DIR="results/par_auto_imp"

# Create folders if they don't exist
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling..."
gcc -std=c99 -O3 -march=native -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO" -lm

# Check compilation status
if [ $? -ne 0 ]; then
    echo "Compilation error. Aborted."
    exit 1
fi
echo "Compilation successful."

# Iterate over matrices and vectors simultaneously
for i in {0..4}; do
    MATRIX_FILE="${MATRICES[$i]}"
    VECTOR_FILE="${VECTORS[$i]}"

    MATRIX="$MATRIX_DIR/$MATRIX_FILE"
    VECTOR="$MATRIX_DIR/$VECTOR_FILE"

    for THREADS in "${THREADS_LIST[@]}"; do
        RESULT_FILE="$RESULTS_DIR/par_auto_imp_${THREADS}_$(basename "$MATRIX_FILE" .mtx).txt"
        echo "Processing matrix $MATRIX with vector $VECTOR using $THREADS threads"

        echo "Results for $MATRIX with $THREADS threads" > "$RESULT_FILE"

        for j in {1..10}; do
            OUT=$("$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR" "$THREADS")
            echo "$OUT" | grep "Elapsed time" | tee -a "$RESULT_FILE"
        done

        echo "Results saved in $RESULT_FILE"
        echo ""
    done
done

echo "ALL MATRICES COMPLETED FOR ALL THREADS"