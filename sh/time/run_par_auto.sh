/********************************************************************************************
 * Description:
 *    Automated execution script for the parallel Sparse Matrix-Vector multiplication
 *    executable `par_auto_spmv`.
 *
 *    This script:
 *        1. Compiles the source code (`par_auto_spmv.c` and `mmio.c`) with OpenMP support.
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
 *    ./run_par_auto.sh
 *
 * Output:
 *    - Compiled executable in `build/` directory
 *    - Elapsed time results for each matrix and thread count in `results/`
 ********************************************************************************************/


# Variables
SOURCE_DIR="./source"           # Directory where the C source code is
MATRIX_DIR="./matrix"           # Directory where the .mtx matrices are
SOURCE="$SOURCE_DIR/par_auto_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="par_auto_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

THREADS_LIST=(1 2 4 8 16 32 64)    # Number of threads to test

BUILD_DIR="build"
RESULTS_DIR="results"

# Create folders if they don't exist
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling..."
gcc -std=c99 -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

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
        RESULT_FILE="$RESULTS_DIR/par_auto_${THREADS}_$(basename "$MATRIX_FILE" .mtx).txt"
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