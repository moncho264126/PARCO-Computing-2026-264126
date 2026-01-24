#!/bin/bash

# ==============================
# Paths and names
# ==============================
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

SOURCE="$SOURCE_DIR/mpi_spmv_concurrent.c"
MMIO="$SOURCE_DIR/mmio.c"
EXE_NAME="mpi_spmv_concurrent"

# ==============================
# Weak scaling configuration
# ==============================
PROCS_LIST=(1 2 4 8 16 32 64 128)

# Asegúrate de que estos nombres coinciden con tus archivos reales
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
mpicc -O3 -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE" "$MMIO"

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
    MATRIX="$MATRIX_DIR/${MATRICES[$i]}"
    VECTOR="$MATRIX_DIR/${VECTORS[$i]}"

    RESULT_FILE="$RESULTS_DIR/weak_concurrent_np_${NP}.txt"

    echo "Running weak concurrent scaling with NP=$NP"
    echo "Matrix: ${MATRICES[$i]}"

    # Encabezado
    echo "Weak concurrent scaling NP=$NP" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((run=1; run<=N_RUNS; run++)); do
        # Ejecutar y capturar toda la salida
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")

        # PARSING EXACTO PARA TU PRINTF
        # Filtramos solo la línea que nos interesa para evitar ruido
        LINE=$(echo "$OUT" | grep "SpMV Total:")

        # Extraemos las columnas exactas basadas en espacios
        # $3 = Total, $7 = Comm, $11 = Comp
        T_TOTAL=$(echo "$LINE" | awk '{print $3}')
        T_COMM=$(echo "$LINE" | awk '{print $7}')
        T_COMP=$(echo "$LINE" | awk '{print $11}')

        # Imprimimos en pantalla para verificar visualmente
        echo "Run $run: $T_TOTAL | $T_COMM | $T_COMP"

        # Guardamos en archivo
        echo "$run | $T_TOTAL | $T_COMM | $T_COMP" >> "$RESULT_FILE"
    done

    echo ""
done

echo "All weak scaling experiments completed."