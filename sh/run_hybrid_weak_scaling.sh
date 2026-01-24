#!/bin/bash

# ==============================
# Paths
# ==============================
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

# Asegúrate de que el nombre coincide con tu archivo C híbrido
SOURCE="$SOURCE_DIR/hybrid_spmv.c"
EXE_NAME="hybrid_spmv"

# ==============================
# Weak scaling Configuration
# ==============================
# VOLVEMOS A LA LISTA ORIGINAL (Potencias de 2)
# Esto asegura que WS1 vaya con np=1 ... y WS8 con np=128
PROCS_LIST=(1 2 4 8 16 32 64 128)

MATRICES=(WS1.mtx WS2.mtx WS3.mtx WS4.mtx WS5.mtx WS6.mtx WS7.mtx WS8.mtx)
VECTORS=(WSV1.mtx WSV2.mtx WSV3.mtx WSV4.mtx WSV5.mtx WSV6.mtx WSV7.mtx WSV8.mtx)

# CONFIGURACIÓN HÍBRIDA
# Usamos 2 hilos por proceso MPI.
# Si usáramos más (ej. 4), necesitaríamos muchísimos más nodos en el PBS.
export OMP_NUM_THREADS=2

N_RUNS=10

# ==============================
# Setup & Compile
# ==============================
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling Hybrid code..."
# Importante: -fopenmp
mpicc -O3 -fopenmp -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

# ==============================
# Run Loop
# ==============================
for i in "${!PROCS_LIST[@]}"; do

    NP=${PROCS_LIST[$i]}
    MATRIX="$MATRIX_DIR/${MATRICES[$i]}"
    VECTOR="$MATRIX_DIR/${VECTORS[$i]}"

    # Nombre del archivo indicando que es híbrido
    RESULT_FILE="$RESULTS_DIR/weak_hybrid_np_${NP}_omp_${OMP_NUM_THREADS}.txt"

    echo "Running Weak Hybrid: NP=$NP (Threads per proc: $OMP_NUM_THREADS)"
    echo "Matrix: ${MATRICES[$i]}"

    echo "Weak Hybrid NP=$NP OMP=$OMP_NUM_THREADS Matrix=${MATRICES[$i]}" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((run=1; run<=N_RUNS; run++)); do
        echo "Run $run / $N_RUNS"

        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")

        # PARSING AJUSTADO AL NUEVO FORMATO C
        # Output C: "SpMV Total: %e s | Comm: %e s | Comp: %e s"
        # Posiciones awk: Total=$3, Comm=$7, Comp=$11
        T_TOTAL=$(echo "$OUT" | grep "SpMV Total" | awk '{print $3}')
        T_COMM=$(echo "$OUT" | grep "SpMV Total" | awk '{print $7}')
        T_COMP=$(echo "$OUT" | grep "SpMV Total" | awk '{print $11}')

        echo "$run | $T_TOTAL | $T_COMM | $T_COMP" | tee -a "$RESULT_FILE"
    done

    echo ""
done

echo "All weak scaling experiments completed."