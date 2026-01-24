#!/bin/bash

SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

SOURCE="$SOURCE_DIR/mpi_spmv_mpiio.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="mpi_spmv_mpiio"

# Archivos de datos
MATRIX="$MATRIX_DIR/SSM.mtx"
VECTOR="$MATRIX_DIR/SSV.mtx"

PROCS_LIST=(1 2 4 8 16 32 64 128)
N_RUNS=10

mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling MPI code..."
mpicc -O3 -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO" -lm
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

for NP in "${PROCS_LIST[@]}"; do
    RESULT_FILE="$RESULTS_DIR/strong_mpiio_np_${NP}.txt"
    echo "Running strong mpiio scaling with NP=$NP"
    echo "Strong mpiio scaling NP=$NP" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"
    
    for ((i=1; i<=N_RUNS; i++)); do
        # IMPORTANTE: Hemos quitado 2>/dev/null para ver errores si crashea
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")
        
        # Check si hubo output de error de MPI antes de parsear
        if echo "$OUT" | grep -q "ERROR"; then
            echo "Run $i FAILED with error:"
            echo "$OUT" | grep "ERROR"
            T_TOTAL="FAIL"
            T_COMM="FAIL"
            T_COMP="FAIL"
        else
            LINE=$(echo "$OUT" | grep "SpMV Total:")
            if [ -n "$LINE" ]; then
                T_TOTAL=$(echo "$LINE" | awk '{print $3}')
                T_COMM=$(echo "$LINE" | awk '{print $7}')
                T_COMP=$(echo "$LINE" | awk '{print $11}')
            else
                T_TOTAL="N/A"
                T_COMM="N/A"
                T_COMP="N/A"
            fi
        fi
        
        echo "$i | $T_TOTAL | $T_COMM | $T_COMP" >> "$RESULT_FILE"
        echo "Run $i: $T_TOTAL | $T_COMM | $T_COMP"
    done
    echo ""
done
echo "Done."