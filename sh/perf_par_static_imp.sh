#!/bin/bash

# ------------------------ VARIABLES ------------------------
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
SOURCE="$SOURCE_DIR/par_static_imp_spmv.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="par_static_imp_spmv"

VECTORS=("v1.mtx" "v2.mtx" "v3.mtx" "v4.mtx" "v5.mtx")
MATRICES=("1.mtx" "2.mtx" "3.mtx" "4.mtx" "5.mtx")

THREADS=(1 2 4 8 16 32 64)
CHUNKSIZES=(1 100 1000)

BUILD_DIR="build"
RESULTS_DIR="results_perf"

# ------------------------ CREAR CARPETAS ------------------------
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ------------------------ COMPILAR ------------------------
echo "Compiling..."
gcc -std=c99 -O3 -march=native -g -Wall -fopenmp -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO" -lm
if [ $? -ne 0 ]; then
    echo "Compilation error. Aborted."
    exit 1
fi
echo "Compilation successful."

# ------------------------ MATRICES A PROCESAR ------------------------
# Puedes cambiar esto para elegir matrices específicas
SELECTED_INDICES=(0 1 2 3 4)

# ------------------------ LOOP PRINCIPAL ------------------------
for idx in "${SELECTED_INDICES[@]}"; do
    MATRIX_FILE="${MATRICES[$idx]}"
    VECTOR_FILE="${VECTORS[$idx]}"

    MATRIX="$MATRIX_DIR/$MATRIX_FILE"
    VECTOR="$MATRIX_DIR/$VECTOR_FILE"

    PCT=$(basename "$MATRIX_FILE" .mtx)

    for THREAD in "${THREADS[@]}"; do
        for CHUNK in "${CHUNKSIZES[@]}"; do

            RESULT_FILE="$RESULTS_DIR/par_static_imp_${THREAD}_${CHUNK}_${PCT}.txt"
            echo "Processing matrix $MATRIX | vector=$VECTOR | threads=$THREAD | chunksize=$CHUNK"

            # Cabecera del archivo
            echo "Performance results for $MATRIX | threads=$THREAD | chunksize=$CHUNK" > "$RESULT_FILE"
            echo "L1-dcache-loads | L1-dcache-load-misses | LLC-loads | LLC-load-misses" >> "$RESULT_FILE"

            # Ejecutar 10 veces
            for _ in {1..10}; do

                PERF_OUT=$(perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses \
                           "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR" "$THREAD" "$CHUNK" \
                           2>&1 1>/dev/null)

                L1_LOADS=$(echo "$PERF_OUT" | grep "L1-dcache-loads" | awk '{print $1}')
                L1_MISSES=$(echo "$PERF_OUT" | grep "L1-dcache-load-misses" | awk '{print $1}')
                LLC_LOADS=$(echo "$PERF_OUT" | grep "LLC-loads" | awk '{print $1}')
                LLC_MISSES=$(echo "$PERF_OUT" | grep "LLC-load-misses" | awk '{print $1}')

                # Guardar contadores (sin número de run)
                echo "$L1_LOADS | $L1_MISSES | $LLC_LOADS | $LLC_MISSES" >> "$RESULT_FILE"
            done

            echo "Results saved in $RESULT_FILE"
            echo ""
        done
    done
done

echo "ALL MATRICES COMPLETED"