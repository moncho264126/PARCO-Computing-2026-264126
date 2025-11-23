#!/bin/bash
/********************************************************************************************
 * Description:
 *    Automated execution script for the parallel Sparse Matrix-Vector multiplication
 *    executable `par_guided_spmv`.
 *
 *    This script:
 *        1. Compiles the source code (`par_guided_spmv.c` and `mmio.c`) with OpenMP support.
 *        2. Iterates over a predefined set of matrices and corresponding vectors.
 *        3. Executes the program for each matrix-vector pair with multiple thread counts.
 *        4. Captures and stores the elapsed execution time for each run.
 *        5. Repeats each run 10 times for statistical averaging.
 *        6. Saves results in the `results/` directory with descriptive filenames.
 *
 * Requirements:
 *    - GCC with OpenMP support
 *    - Source files located in `./source`
 *    - Matrix and vector files located in `./matrix`
 *
 * Usage:
 *    ./run_par_guided.sh
 *
 * Output:
 *    - Compiled executable in `build/` directory
 *    - Elapsed time results for each matrix and thread count in `results/`
 ********************************************************************************************/

SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
SOURCE="$SOURCE_DIR/par_guided_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="par_guided_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

BUILD_DIR="build"
RESULTS_DIR="results"

THREADS=(1 2 4 8 16 32 64)
CHUNKSIZES=(1 100 1000)

mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling..."
gcc -std=c99 -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

if [ $? -ne 0 ]; then
    echo "Compilation error. Aborted."
    exit 1
fi
echo "Compilation successful."

SELECTED_INDICES=(0 1 2 3 4)
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