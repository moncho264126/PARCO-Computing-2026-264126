#!/bin/bash

# ==============================
# Paths and names
# ==============================
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

# OJO: Cambiado el nombre del fuente y eliminado mmio.c
SOURCE="$SOURCE_DIR/hybrid_spmv.c"
EXE_NAME="hybrid_spmv"

MATRIX="$MATRIX_DIR/SSM.mtx"
VECTOR="$MATRIX_DIR/SSV.mtx"

# ==============================
# HYBRID CONFIGURATION
# ==============================
# Antes llegabas a 128 procesos. 
# Ahora, como usaremos 8 hilos por proceso, dividimos los rangos entre 8.
# 1 MPI (8 hilos) ~= 8 núcleos
# 16 MPI (8 hilos) ~= 128 núcleos
PROCS_LIST=(1 2 4 8 16)

# Definimos los hilos OMP fijos para este experimento
export OMP_NUM_THREADS=8

N_RUNS=10

# ==============================
# Create directories
# ==============================
mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

# ==============================
# Compile (Con -fopenmp y sin mmio.c)
# ==============================
echo "Compiling Hybrid MPI+OpenMP code..."
mpicc -O3 -fopenmp -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

# ==============================
# Run experiments
# ==============================
for NP in "${PROCS_LIST[@]}"; do
    # Calculamos cores totales para nombrar el archivo correctamente
    TOTAL_CORES=$((NP * OMP_NUM_THREADS))
    RESULT_FILE="$RESULTS_DIR/strong_hybrid_cores_${TOTAL_CORES}.txt"
    
    echo "Running Strong Scaling Hybrid: NP=$NP x OMP=$OMP_NUM_THREADS (Total Cores: $TOTAL_CORES)"

    echo "Strong Scaling Hybrid NP=$NP OMP=$OMP_NUM_THREADS" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((i=1; i<=N_RUNS; i++)); do
        echo "Run $i / $N_RUNS"

        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")
        
        # Parseo actualizado para el formato:
        # "SpMV Total: 1.2e-3 s | Comm: 4.5e-4 s | Comp: 6.7e-4 s"
        T_TOTAL=$(echo "$OUT" | grep "SpMV Total" | awk '{print $3}')
        T_COMM=$(echo "$OUT" | grep "SpMV Total" | awk '{print $7}')
        T_COMP=$(echo "$OUT" | grep "SpMV Total" | awk '{print $11}')

        echo "$i | $T_TOTAL | $T_COMM | $T_COMP" | tee -a "$RESULT_FILE"
    done
    echo ""
done

echo "Hybrid Strong scaling completed."