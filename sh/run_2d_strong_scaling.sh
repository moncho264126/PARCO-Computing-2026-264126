#!/bin/bash

# ==============================
# Paths
# ==============================
SOURCE_DIR="./source"
MATRIX_DIR="./matrix"
BUILD_DIR="./build"
RESULTS_DIR="./results"

SOURCE="$SOURCE_DIR/hybrid_spmv.c"
EXE_NAME="hybrid_spmv"

# Matriz fija para Strong Scaling
MATRIX="$MATRIX_DIR/SSM.mtx"
VECTOR="$MATRIX_DIR/SSV.mtx"

# ==============================
# CONFIGURACIÓN HÍBRIDA STRONG
# ==============================
# Queremos llegar a 128 Cores Totales.
# Como usaremos 8 hilos por proceso (llenar un nodo con 1 MPI):
# 1 Proc  * 8 Hilos = 8 Cores
# ...
# 16 Proc * 8 Hilos = 128 Cores
PROCS_LIST=(1 2 4 8 16)

# Fijamos 8 hilos (asumiendo que tus nodos tienen 8 cpus o múltiplos)
export OMP_NUM_THREADS=8

N_RUNS=10

mkdir -p "$BUILD_DIR"
mkdir -p "$RESULTS_DIR"

echo "Compiling Hybrid code..."
mpicc -O3 -fopenmp -std=c99 -o "$BUILD_DIR/$EXE_NAME" "$SOURCE"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi
echo "Compilation OK"

# ==============================
# Bucle de Ejecución
# ==============================
for NP in "${PROCS_LIST[@]}"; do
    
    # Calculamos cores totales para que el nombre del archivo sea claro
    TOTAL_CORES=$((NP * OMP_NUM_THREADS))
    RESULT_FILE="$RESULTS_DIR/strong_hybrid_cores_${TOTAL_CORES}.txt"

    echo "Running Strong Hybrid: NP=$NP x OMP=$OMP_NUM_THREADS (Total Cores: $TOTAL_CORES)"

    echo "Strong Hybrid NP=$NP OMP=$OMP_NUM_THREADS" > "$RESULT_FILE"
    echo "Run | Total_Time | Communication | Computation" >> "$RESULT_FILE"

    for ((i=1; i<=N_RUNS; i++)); do
        echo "Run $i / $N_RUNS"

        # Ejecución
        OUT=$(mpirun -np "$NP" "$BUILD_DIR/$EXE_NAME" "$MATRIX" "$VECTOR")
        
        # Parseo (Ajustado a tu printf en C)
        # Formato esperado: "SpMV Total: 1.23e-02 s | Comm: ... | Comp: ..."
        # $3 es Total, $7 es Comm, $11 es Comp
        T_TOTAL=$(echo "$OUT" | grep "SpMV Total" | awk '{print $3}')
        T_COMM=$(echo "$OUT" | grep "SpMV Total" | awk '{print $7}')
        T_COMP=$(echo "$OUT" | grep "SpMV Total" | awk '{print $11}')

        echo "$i | $T_TOTAL | $T_COMM | $T_COMP" | tee -a "$RESULT_FILE"
    done
    echo ""
done

echo "Strong scaling experiments completed."