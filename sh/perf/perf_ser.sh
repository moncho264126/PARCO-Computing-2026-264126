/********************************************************************************************
 * Description:
 *    Automated benchmarking script for the serial (single-threaded) Sparse Matrix-Vector
 *    multiplication executable `ser_spmv`.
 *
 *    This script:
 *        1. Compiles the source code (`ser_spmv.c` and `mmio.c`) with OpenMP support enabled.
 *        2. Iterates over a set of matrices and corresponding vectors.
 *        3. Executes the compiled program for each matrix-vector pair.
 *        4. Collects hardware performance counters (L1/L3 cache loads and misses) using `perf`.
 *        5. Repeats each run 10 times for statistical consistency.
 *        6. Stores results in the `results_perf/` directory with descriptive filenames.
 *
 * Requirements:
 *    - GCC with OpenMP support
 *    - Linux environment with `perf` tool installed
 *    - Source files located in `./source`
 *    - Matrix and vector files located in `./matrix`
 *
 * Usage:
 *    ./perf_ser.sh
 *
 * Output:
 *    - Compiled executable in `build/` directory
 *    - Performance results for each matrix in `results_perf/`
 ********************************************************************************************/


# ------------------------ VARIABLES ------------------------
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
SOURCE="$SOURCE_DIR/ser_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="ser_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

BUILD_DIR="build"
RESULTS_DIR="results_perf"

# ------------------------ CREATE DIRECTORIES ------------------------
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ------------------------ COMPILE ------------------------
echo "Compiling..."
gcc -std=c99 -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"
if [ $? -ne 0 ]; then
    echo "Compilation error. Aborted."
    exit 1
fi
echo "Compilation successful."

# ------------------------ LOOP ------------------------
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