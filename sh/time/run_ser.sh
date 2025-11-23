/********************************************************************************************
 * Description:
 *    Automated execution script for the serial (single-threaded) Sparse Matrix-Vector
 *    multiplication executable `ser_spmv`.
 *
 *    This script:
 *        1. Compiles the source code (`ser_spmv.c` and `mmio.c`) with OpenMP support
 *           enabled (though the program runs serially).
 *        2. Iterates over a predefined set of matrices and corresponding vectors.
 *        3. Executes the program for each matrix-vector pair.
 *        4. Captures and stores the elapsed execution time for each run.
 *        5. Repeats each run 10 times for statistical consistency.
 *        6. Saves results in the `results/` directory with descriptive filenames.
 *
 * Requirements:
 *    - GCC with OpenMP support
 *    - Source files located in `./source`
 *    - Matrix and vector files located in `./matrix`
 *
 * Usage:
 *    ./run_ser.sh
 *
 * Output:
 *    - Compiled executable in `build/` directory
 *    - Elapsed time results for each matrix in `results/`
 ********************************************************************************************/


# Variables
SOURCE_DIR="./source"           # Directory where the C source code is
MATRIX_DIR="./matrix"           # Directory where the .mtx matrices are
SOURCE="$SOURCE_DIR/ser_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="ser_spmv"

MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")
VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")

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

# Iterate over the list of matrices **matched with their vector**
for index in "${!MATRICES[@]}"; do

    MATRIX_FILE="${MATRICES[$index]}"
    VECTOR_FILE="${VECTORS[$index]}"

    MATRIX="$MATRIX_DIR/$MATRIX_FILE"
    VECTOR="$MATRIX_DIR/$VECTOR_FILE"

    RESULT_FILE="$RESULTS_DIR/ser_$(basename "$MATRIX_FILE" .mtx).txt"

    echo "Processing matrix $MATRIX with vector $VECTOR"

    # Write header to the results file (overwrite previous content)
    echo "Results for $MATRIX" > "$RESULT_FILE"

    # Run the executable 10 times for the current pair
    for i in {1..10}; do
        OUT=$("$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")

        TIME=$(echo "$OUT" | grep "Elapsed time")

        echo "$TIME" | tee -a "$RESULT_FILE"
    done

    echo "Results saved in $RESULT_FILE"
    echo ""
done

echo "ALL MATRICES COMPLETED"