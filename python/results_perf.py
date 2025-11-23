#!/usr/bin/env python3
"""
===============================================================================
      INTERACTIVE PLOTTING OF HARDWARE PERFORMANCE COUNTERS
===============================================================================

Script: plot_perf_counters.py
Description:
    Provides interactive plots of hardware performance counters extracted
    from SpMV simulations stored in ../results_perf/* directories.

Features:
1. X-axis limited to standard thread counts: 1,2,4,8,16,32,64.
2. Prompts the user to select a matrix and a chunksize.
3. Displays P90 values of counters with different markers per schedule.
4. Prints gains (%) relative to SERIAL and SERIAL_IMP baselines.
5. Supports counters: "L1_loads", "L1_misses", "LLC_loads", "LLC_misses".

Expected directory structure (relative to script):

results_perf/
 ├── ser/
 │     ser_1.txt
 │     ser_2.txt
 ├── ser_imp/
 │     ser_imp_1.txt
 ├── par_auto/
 │     par_auto_<threads>_<matrix>.txt
 ├── par_static/
 │     par_static_<threads>_<chunk>_<matrix>.txt
 ├── par_dynamic/
 │     ...
 └── ...

Usage:
    python plot_perf_counters.py
    - Lists available matrices
    - Prompts for matrix and chunksize
    - Plots P90 values of counters vs threads with schedule markers
    - Prints gains relative to SERIAL and SERIAL_IMP

Dependencies:
    - Python 3.x
    - numpy
    - matplotlib
    - parser_perf.py (must be in the same folder or importable)
===============================================================================
"""

import numpy as np
import matplotlib.pyplot as plt
from parser_perf import parse_all_results

# Load parsed data
ser, ser_imp, par = parse_all_results()

counters = ["L1_loads", "L1_misses", "LLC_loads", "LLC_misses"]
THREADS_STANDARD = [1, 2, 4, 8, 16, 32, 64]
SCHEDULES = list(par.keys())

# Símbolos para cada schedule
markers = {
    "ser": "s",        # cuadrado
    "ser_imp": "D",    # diamante
    "auto": "o",
    "auto_imp": "^",
    "static": "v",
    "static_imp": "<",
    "guided": ">",
    "guided_imp": "p",
    "dynamic": "*",
    "dynamic_imp": "h",
    "runtime": "X",
    "runtime_imp": "+"
}


def percentile(values):
    return np.percentile(values, 90) if len(values) else np.nan


def plot_counter_vs_threads(matrix, chunksize, counter):
    plt.figure(figsize=(10, 6))

    # Serial reference
    if matrix in ser:
        ser_val = percentile(ser[matrix][counter])
        plt.hlines(ser_val, min(THREADS_STANDARD), max(THREADS_STANDARD),
                   linestyle="--", color="black", label="SERIAL")
    if matrix in ser_imp:
        ser_imp_val = percentile(ser_imp[matrix][counter])
        plt.hlines(ser_imp_val, min(THREADS_STANDARD), max(THREADS_STANDARD),
                   linestyle="--", color="red", label="SERIAL_IMP")

    print(f"\nCounter: {counter} – Matrix {matrix}, Chunk {chunksize}")
    print("Threads | Schedule | P90 Value | Ganancia vs SERIAL (%) | Ganancia vs SERIAL_IMP (%)")

    for sched in SCHEDULES:
        xs = []
        ys = []
        for t in THREADS_STANDARD:
            data = par[sched].get(t, {}).get(matrix, None)
            if data is None:
                xs.append(t)
                ys.append(np.nan)
                continue

            # Auto schedules (sin chunk)
            if sched in ["auto", "auto_imp"]:
                arr = data[counter]
            else:
                # Sólo usar chunk si coincide con el seleccionado
                if chunksize in data:
                    arr = data[chunksize][counter]
                else:
                    xs.append(t)
                    ys.append(np.nan)
                    continue

            p90_val = percentile(arr)
            xs.append(t)
            ys.append(p90_val)

            # Ganancias respecto a SERIAL y SERIAL_IMP
            gain_ser = ((ser_val - p90_val) / ser_val * 100) if matrix in ser else np.nan
            gain_ser_imp = ((ser_imp_val - p90_val) / ser_imp_val * 100) if matrix in ser_imp else np.nan
            print(f"{t:7d} | {sched.upper():10} | {p90_val:10.0f} | {gain_ser:7.2f} | {gain_ser_imp:7.2f}")

        plt.plot(xs, ys, marker=markers.get(sched, "o"), label=sched.upper())

    plt.title(f"{counter} – Matrix {matrix}, Chunk {chunksize}")
    plt.xlabel("Threads")
    plt.ylabel(f"P90 {counter}")
    plt.xticks(THREADS_STANDARD)
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.show()


# -------------------------------------------------
# Interfaz simple
# -------------------------------------------------
print("Matrices disponibles (según datos):")
matrices_disponibles = sorted({int(m) for sched in par
                               for t in par[sched]
                               for m in par[sched][t]})
print(matrices_disponibles)

matrix = int(input("Introduce el número de matriz: "))

# Sólo schedules con chunks para evitar keys como "L1_loads"
chunksizes_disponibles = sorted({int(c) for sched in par if sched not in ["auto", "auto_imp"]
                                 for t in par[sched]
                                 for m in par[sched][t]
                                 for c in par[sched][t][m].keys()})
print("Chunksizes disponibles (para schedules con chunks):", chunksizes_disponibles)

chunksize = int(input("Introduce el chunksize: "))

# Dibujar todas las curvas para cada counter
for counter in counters:
    plot_counter_vs_threads(matrix, chunksize, counter)