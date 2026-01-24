#!/bin/bash

# ==============================
# Paths and names
# ==============================
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

# CAMBIO: Apunta al nuevo archivo 2D
SOURCE="$SOURCE_DIR/mpi_spmv_2d.c"
EXE_NAME="mpi_spmv_2d_weak"

# ==============================
# Weak scaling configuration
# ==============================
PROCS_LIST=(1 2 4 8 16 32 64 128)

# Asegúrate de que estos archivos existen en tu carpeta matrix
MATRICES=(WS1.mtx WS2.mtx WS3.mtx WS4.mtx WS5.mtx WS6.mtx WS7.mtx WS8.mtx)
VECTORS=(WSV1.mtx WSV2.mtx WSV3.mtx WSV4.mtx WSV5.mtx WSV6.mtx WSV7.mtx WSV8.mtx)

N_RUNS=10

# ==============================
# Create directories
# ==============================
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ==============================
# Compile
# ==============================
echo "Compiling MPI code..."
# CAMBIO: Quitamos "$MMIO" de la compilación
mpicc -O3 -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" -lm

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

# ==============================
# Run weak scaling experiments
# ==============================
for i in "${!PROCS_LIST[@]}"; do

    NP=${PROCS_LIST[$i]}
    # Verificación de que existen índices suficientes
    if [ -z "${MATRICES[$i]}" ]; then
        echo "Error: No hay matriz definida para el índice $i (NP=$NP)"
        break
    fi

    MATRIX="$MATRIX_DIR/${MATRICES[$i]}"
    VECTOR="$MATRIX_DIR/${VECTORS[$i]}"

    RESULT_FILE="$RESULTS_DIR/weak_2d_np_${NP}.txt"

    echo "Running weak scaling 2D with NP=$NP"
    echo "Matrix: ${MATRICES[$i]}"

    # Encabezado
    echo "Weak 2D scaling NP=$NP" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((run=1; run<=N_RUNS; run++)); do
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")

        LINE=$(echo "$OUT" | grep "SpMV Total:")
        
        if [ -z "$LINE" ]; then
             echo "Run $run: FALLO o sin output de tiempos."
             T_TOTAL="FAIL"
             T_COMM="FAIL"
             T_COMP="FAIL"
        else
             T_TOTAL=$(echo "$LINE" | awk '{print $3}')
             T_COMM=$(echo "$LINE" | awk '{print $7}')
             T_COMP=$(echo "$LINE" | awk '{print $11}')
             echo "Run $run: $T_TOTAL | $T_COMM | $T_COMP"
        fi

        echo "$run | $T_TOTAL | $T_COMM | $T_COMP" >> "$RESULT_FILE"
    done
    echo ""
done

echo "All weak scaling experiments completed."