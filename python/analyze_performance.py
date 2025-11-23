#!/usr/bin/env python3
"""
===============================================================================
    PERFORMANCE ANALYSIS & VISUALIZATION OF PARALLEL SCHEDULING METHODS
===============================================================================

Description:
    This script provides comprehensive analysis and visualization of execution 
    times (P90) for different parallel scheduling methods, including:
        - Serial, Serial with implicit optimizations
        - Parallel schedules: static, guided, dynamic, runtime (with/without implicit optimizations)
    It evaluates multiple matrix sizes and thread counts.

Main Features:
    1. Accumulates P90 timings across:
        - All matrices
        - Small matrices (1 & 2)
        - Large matrices (3, 4, 5)
    2. Generates optional visualizations:
        - Base bar charts comparing all methods per matrix, thread, and chunk size
        - Heatmaps of average P90 values by scheduling method
        - Average speedup per method relative to SERIAL and SERIAL_IMP
        - Variability (standard deviation) per method
        - GFLOPS calculation (requires matrix files in ../matrix/*.mtx)
        - Scaling efficiency plots
        - Speedup / Efficiency / P90 Time vs. number of threads per matrix
    3. Computes rankings of methods based on average P90 timings.
    4. Interactive menu for selective visualization and queries, including:
        - GFLOPS
        - Parallel efficiency
        - Roofline model analysis
        - Comparison between SERIAL and SERIAL_IMP

Requirements:
    - Python 3.x
    - matplotlib, numpy
    - Optional: seaborn (for heatmaps)
    - Matrix files in ../matrix/*.mtx
    - Results parsed via parser.py functions: parse_results, extract_std

Usage:
    $ python3 analyze_results.py
    - Follow interactive menu for desired analysis and plots.

Notes:
    - All results are P90 timings (90th percentile) of multiple runs.
    - Chunk size handling differs for 'auto' schedules.
    - Hardware parameters (GFLOPS, memory bandwidth) can be adjusted
      according to your system.
"""


import os
import matplotlib.pyplot as plt
import numpy as np
from parser import parse_results, extract_std
from collections import defaultdict

# ---------------------------
# INTERACTIVE MAIN MENU
# ---------------------------
print("\n============================")
print("       GRAPH OPTIONS        ")
print("============================")
print("0. Show global rankings only (no plots)")
print("1. Base bar charts")
print("2. Heatmaps by schedule")
print("3. Average speedup bars")
print("4. Variability bars")
print("5. GFLOPS")
print("6. Efficiency")
print("7. Speedup vs threads")
print("8. Efficiency vs threads")
print("9. P90 Time vs threads")
print("10. Roofline Model")
print("11. Serial comparison")
print("============================")

choice = input("Select an option (0-11): ").strip()
SHOW_BASE            = (choice == "1")
SHOW_HEATMAP         = (choice == "2")
SHOW_SPEEDUP         = (choice == "3")
SHOW_VAR             = (choice == "4")
SHOW_GFLOPS          = (choice == "5")
SHOW_EFF             = (choice == "6")
SHOW_SPEEDUP_THREADS = (choice == "7")
SHOW_EFF_THREADS     = (choice == "8")
SHOW_TIME_THREADS    = (choice == "9")
SHOW_ROOFLINE        = (choice == "10")
SHOW_SER_COMPAR      = (choice == "11")


# ---------------------------
# LOAD RESULTS FROM PARSER
# ---------------------------
ser, ser_imp, par = parse_results()

schedules = [
    "auto", "auto_imp",
    "static", "static_imp",
    "guided", "guided_imp",
    "dynamic", "dynamic_imp",
    "runtime", "runtime_imp"
]

# THREADS and MATRICES
THREADS = sorted({t for s in schedules for t in par.get(s, {}).keys()})
mat_set = set()
for s in schedules:
    for tdict in par.get(s, {}).values():
        mat_set |= set(tdict.keys())
MATRICES = sorted(mat_set)

# CHUNKS: canonical options
DEFAULT_CHUNKS = [1, 100, 1000]
CANONICAL_CHUNKS = ["0", "1", "100", "1000"]

# ---------------------------
# HELPERS
# ---------------------------
def get_p90(schedule_dict, threads, matrix, chunk=None):
    if not schedule_dict or threads not in schedule_dict or matrix not in schedule_dict[threads]:
        return None
    val = schedule_dict[threads][matrix]
    if chunk is None:
        if isinstance(val, dict):
            keys = sorted(val.keys())
            return val[keys[0]] if keys else None
        return val
    if isinstance(val, dict):
        return val.get(chunk, None)
    return None

# -----------------------------
# Parámetros de tu máquina
# -----------------------------
PEAK_GFLOPS = 1177.0    # Calculado para 32 cores Xeon Gold 5118 @2.3GHz
MEM_BW_GB_S = 50.0      # Ancho de banda memoria del cluster
BYTES_PER_FLOAT = 8     # doble precisión
BYTES_PER_INDEX = 4     # entero 32-bit para índices

# ---------------------------
# RANKINGS
# ---------------------------
def rank(acc_dict, desc):
    avg = {k: np.mean(v) for k, v in acc_dict.items() if v}
    sorted_avg = sorted(avg.items(), key=lambda x: x[1])
    print(f"\nRanking: {desc}")
    for i, (name, val) in enumerate(sorted_avg, 1):
        print(f"{i}. {name:15s} → {val:.3f} ms")

acc_all = defaultdict(list)
acc_small = defaultdict(list)
acc_large = defaultdict(list)

for m in MATRICES:
    for t in THREADS:
        for c in DEFAULT_CHUNKS:
            methods = {}
            if ser.get(m): methods["SERIAL"] = ser[m]
            if ser_imp.get(m): methods["SERIAL_IMP"] = ser_imp[m]
            for sched in schedules:
                sched_dict = par.get(sched, {})
                p90 = get_p90(sched_dict, t, m, None if "auto" in sched else c)
                if p90 is not None:
                    methods[sched.upper()] = p90
            if not methods:
                continue
            for k, v in methods.items():
                acc_all[k].append(v)
                if m in [1, 2]:
                    acc_small[k].append(v)
                if m in [3, 4, 5]:
                    acc_large[k].append(v)

rank(acc_all,   "All matrices (global average)")
rank(acc_small, "Small matrices (1 & 2)")
rank(acc_large, "Large matrices (3,4,5)")

if choice == "0":
    print("\nNo plots requested. Exiting.\n")
    exit(0)

# ---------------------------
# NNZ / GFLOPS / EFFICIENCY
# ---------------------------
NNZ_TABLE = {1:900000,2:4500000,3:182082942,4:225422112,5:270234840}

def compute_gflops(nnz, time_ms):
    if nnz is None or time_ms is None or time_ms <= 0:
        return None
    flops = 2.0 * nnz
    return (flops / (time_ms / 1000.0)) / 1e9

def compute_efficiency(speedup, threads):
    if threads is None or threads <= 0:
        return None
    return speedup / float(threads)

def ask_choice(label, options):
    opt_strs = [str(x) for x in options]
    print(f"\nSelect {label}:")
    print("Available:", opt_strs)
    while True:
        x = input("> ").strip()
        try:
            xi = int(x)
            if str(xi) in opt_strs: return xi
        except ValueError:
            if x in opt_strs: return x
        print("Invalid, try again.")

def evaluate_point(metric):
    mat_opts = [int(m) for m in MATRICES]
    m = ask_choice("matrix (id)", mat_opts)
    t = ask_choice("threads", THREADS)
    chunk_options = [1, 100, 1000]
    c = ask_choice("chunk size", chunk_options)
    print(f"\n--- Results for Matrix {m}, Threads {t}, Chunk {'auto' if c is None else c} ---\n")
    nnz = NNZ_TABLE.get(int(m), None)

    labels, values = [], []
    for sched in schedules:
        sched_dict = par.get(sched, {})
        p90 = get_p90(sched_dict, t, m, None if "auto" in sched else c)
        if p90 is None: continue
        if metric=="gflops":
            g = compute_gflops(nnz, p90)
            if g is not None:
                labels.append(sched.upper())
                values.append(g)
                print(f"{sched.upper():15s} → {g:.3f} GFLOPS (P90 {p90:.3f} ms)")
        else:
            base_ser = ser.get(m)
            if base_ser is None: continue
            speedup = base_ser / p90
            eff = compute_efficiency(speedup, t)
            labels.append(sched.upper())
            values.append(eff)
            print(f"{sched.upper():15s} → {eff*100:.2f}% efficiency (speedup {speedup:.2f})")
    if labels and values:
        plt.figure(figsize=(10,5))
        plt.bar(labels, values)
        plt.xticks(rotation=45)
        if metric=="gflops":
            plt.ylabel("GFLOPS")
            plt.title(f"GFLOPS — Matrix {m} | Threads {t} | Chunk {'auto' if c is None else c}")
        else:
            plt.ylabel("Efficiency")
            plt.title(f"Parallel Efficiency — Matrix {m} | Threads {t} | Chunk {'auto' if c is None else c}")
        plt.grid(axis="y", linestyle="--", alpha=0.5)
        plt.tight_layout()
        plt.show()
    else:
        print("No data available to plot for this selection.")

# ---------------------------
# MARKERS POR SCHEDULE
# ---------------------------
MARKERS = {
    "auto": "o", "auto_imp": "x",
    "static": "s", "static_imp": "D",
    "guided": "^", "guided_imp": "v",
    "dynamic": "<", "dynamic_imp": ">",
    "runtime": "p", "runtime_imp": "*"
}

# ---------------------------
# SPEEDUP / EFFICIENCY / TIME vs THREADS
# PROMEDIO SOBRE CHUNKS, IGNORANDO chunk
# ---------------------------
def plot_speedup_vs_threads(matrix, chunk):
    plt.figure(figsize=(10,6))
    for sched in schedules:
        times = [get_p90(par.get(sched, {}), t, matrix, None if "auto" in sched else chunk) for t in THREADS]
        base_ser = ser.get(matrix)
        speedup_ser = np.array([base_ser/x if x else np.nan for x in times])
        plt.plot(THREADS, speedup_ser, marker=MARKERS[sched], label=f"{sched.upper()}")
    plt.xticks(THREADS)
    plt.xlabel("Threads")
    plt.ylabel("Speedup")
    plt.title(f"Speedup vs Threads — Matrix {matrix} | Chunk {chunk}")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.show()

def plot_efficiency_vs_threads(matrix, chunk):
    plt.figure(figsize=(10,6))
    for sched in schedules:
        effs = []
        for t in THREADS:
            t_val = get_p90(par.get(sched, {}), t, matrix, None if "auto" in sched else chunk)
            if t_val is not None and ser.get(matrix):
                speedup = ser[matrix] / t_val
                effs.append(compute_efficiency(speedup, t))
            else:
                effs.append(np.nan)
        plt.plot(THREADS, effs, marker=MARKERS[sched], label=sched.upper())
    plt.xticks(THREADS)
    plt.xlabel("Threads")
    plt.ylabel("Parallel Efficiency")
    plt.title(f"Efficiency vs Threads — Matrix {matrix} | Chunk {chunk}")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.show()

def plot_time_vs_threads(matrix, chunk):
    plt.figure(figsize=(10,6))
    
    ser_time = ser.get(matrix)
    if ser_time:
        plt.hlines(ser_time, min(THREADS), max(THREADS), color='k', linestyle='--', alpha=0.5, label='SERIAL Time')

    for sched in schedules:
        times, stds = [], []
        folder_name = f"par_{sched}" 

        for t in THREADS:
            p90 = get_p90(par.get(sched, {}), t, matrix, None if "auto" in sched else chunk)
            if "auto" in sched:
                fname = f"par_{sched}_{t}_{matrix}.txt"
            else:
                fname = f"par_{sched}_{t}_{chunk}_{matrix}.txt" 

            path = os.path.join("..", "results", folder_name, fname)
            std = extract_std(path)
            times.append(p90 if p90 is not None else np.nan)
            stds.append(std if std is not None else 0.0)
            
        plt.errorbar(THREADS, times, yerr=stds, marker=MARKERS[sched], label=sched.upper(), capsize=3)
        
    plt.xticks(THREADS)
    plt.xlabel("Threads")
    plt.ylabel("P90 Time $\\pm$ STD (ms)")
    plt.title(f"P90 Time $\\pm$STD vs Threads — Matrix {matrix} | Chunk {chunk}")
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.show()

def plot_roofline(matrix, chunk, scheduler_to_plot):
    
    # ---------------------------------------------
    # CONSTANTES DE HARDWARE (usando tus valores: 1177 GFLOPS y 50 GB/s)
    # ---------------------------------------------
    R_PEAK = 1177.0  # Pico de Rendimiento (GFLOPS)
    B_PEAK = 50.0    # Pico de Ancho de Banda (GB/s)
    
    nnz = NNZ_TABLE.get(matrix)
    if nnz is None:
        print(f"Error: NNZ no encontrado para la matriz {matrix}.")
        return

    flops = 2 * nnz
    bytes_moved = nnz * (BYTES_PER_FLOAT + BYTES_PER_INDEX) 
    ai = flops / bytes_moved 

    # -----------------------------
    # Config
    # -----------------------------
    AI_CRITICO = R_PEAK / B_PEAK
    x = np.logspace(-2, 3, 500)
    roof_mem_gflops = B_PEAK * x 
    roof_cpu_gflops = np.full_like(x, R_PEAK)
    
    plt.figure(figsize=(10,6))
    plt.loglog(x, np.minimum(roof_mem_gflops, roof_cpu_gflops), 'k-', linewidth=3, label='Roofline Model (Upper Bound)')
    plt.loglog(x, roof_mem_gflops, 'r--', label=f'Memory-bound ({B_PEAK} GB/s)', alpha=0.6)
    plt.loglog(x, roof_cpu_gflops, 'b--', label=f'Compute-bound ({R_PEAK} GFLOPS)', alpha=0.6)
    
    plt.axvline(ai, color='g', linestyle='-.', alpha=0.7, label=f'AI SpMV ({ai:.3f} FLOP/Byte)')

    # -----------------------------
    # Plot
    # -----------------------------
    points_gflops = []
    points_ai = []
    
    for t in THREADS:
        c = None if "auto" in scheduler_to_plot else chunk
        p90 = get_p90(par.get(scheduler_to_plot, {}), t, matrix, c)
        
        if p90 is not None and p90 > 0:
            gflops_val = compute_gflops(nnz, p90)
            
            if gflops_val is not None:
                points_ai.append(ai)
                points_gflops.append(gflops_val)
                
                plt.scatter(ai, gflops_val, 
                            marker=MARKERS.get(scheduler_to_plot, 'o'), 
                            s=60, 
                            color='black',
                            alpha=0.8)
                
                plt.annotate(
                    f"t={t} ({gflops_val:.2f} GFLOPS)", 
                    (ai, gflops_val), 
                    xytext=(5, 5), 
                    textcoords='offset points', 
                    fontsize=8
                )

    # -----------------------------
    # Config
    # -----------------------------
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Intensidad Aritmética [FLOP/byte]')
    plt.ylabel('Performance [GFLOPS]')
    plt.title(f"Roofline — Matrix {matrix} | Scheduler: {scheduler_to_plot.upper()} | Chunk: {chunk}")
    plt.grid(True, which="both", linestyle="--", alpha=0.5)
    plt.xlim(x[0], x[-1])
    plt.ylim(1, R_PEAK * 1.5)
    plt.legend(loc='lower right', fontsize='small')
    plt.tight_layout()
    plt.show()

# ---------------------------
# DISPATCH MODE
# ---------------------------
if SHOW_GFLOPS:
    print("\n=== GFLOPS FOR ALL COMBINATIONS ===\n")
    for m in MATRICES:
        nnz = NNZ_TABLE.get(int(m), None)
        for t in THREADS:
            for c in DEFAULT_CHUNKS:
                print(f"\nMatrix {m}, Threads {t}, Chunk {c}")
                for sched in schedules:
                    sched_dict = par.get(sched, {})
                    p90 = get_p90(sched_dict, t, m, None if "auto" in sched else c)
                    if p90 is None: 
                        print(f"{sched.upper():15s} → No data")
                        continue
                    gflops = compute_gflops(nnz, p90)
                    if gflops is not None:
                        print(f"{sched.upper():15s} → {gflops:.3f} GFLOPS (P90 {p90:.3f} ms)")
                    else:
                        print(f"{sched.upper():15s} → Invalid GFLOPS")
elif SHOW_HEATMAP:
    try:
        import seaborn as sns
    except Exception:
        print("Seaborn not installed; heatmaps skipped.")
        sns = None

    if sns is not None:
        m = ask_choice("matrix (id)", MATRICES)
        s = ask_choice("schedule", schedules)
        schedule_upper = s.upper()

        heat = np.full((len(THREADS), len(DEFAULT_CHUNKS)), np.nan)
        for i, t in enumerate(THREADS):
            for j, c in enumerate(DEFAULT_CHUNKS):
                p90 = get_p90(par.get(s, {}), t, m, c)
                if p90 is not None:
                    heat[i, j] = p90

        plt.figure(figsize=(10,7))
        sns.heatmap(heat, annot=True, fmt=".2f",
                    xticklabels=DEFAULT_CHUNKS,
                    yticklabels=THREADS,
                    cmap="viridis")
        plt.xlabel("Chunk Size")
        plt.ylabel("Threads")
        plt.title(f"Heatmap — Matrix {m} | Schedule {schedule_upper}")
        plt.tight_layout()
        plt.show()
elif SHOW_SPEEDUP:
    avg_speedups_ser = {}
    avg_speedups_ser_imp = {}
    for sched in schedules:
        vals_ser, vals_ser_imp = [], []
        for m in MATRICES:
            for t in THREADS:
                for c in DEFAULT_CHUNKS:
                    v = get_p90(par.get(sched, {}), t, m, None if "auto" in sched else c)
                    if v:
                        if ser.get(m): vals_ser.append(ser[m]/v)
                        if ser_imp.get(m): vals_ser_imp.append(ser_imp[m]/v)
        if vals_ser: avg_speedups_ser[sched.upper()] = np.mean(vals_ser)
        if vals_ser_imp: avg_speedups_ser_imp[sched.upper()] = np.mean(vals_ser_imp)
    labels = list(avg_speedups_ser.keys())
    x = np.arange(len(labels))
    width = 0.35
    plt.figure(figsize=(12,6))
    plt.bar(x - width/2, list(avg_speedups_ser.values()), width, label="vs SERIAL")
    plt.bar(x + width/2, list(avg_speedups_ser_imp.values()), width, label="vs SERIAL_IMP")
    plt.xticks(x, labels, rotation=45)
    plt.ylabel("Average Speedup")
    plt.title("Average speedup per method")
    plt.legend()
    plt.grid(axis="y", linestyle="--", alpha=0.5)
    plt.tight_layout()
    plt.show()
elif SHOW_VAR:
    method_std = {}
    for sched in schedules:
        allvals = [get_p90(par.get(sched, {}), t, m, None if "auto" in sched else c)
                   for m in MATRICES for t in THREADS for c in DEFAULT_CHUNKS]
        allvals = [v for v in allvals if v is not None]
        if allvals: method_std[sched.upper()] = np.std(allvals)
    labels = list(method_std.keys())
    values = list(method_std.values())
    plt.figure(figsize=(10,5))
    plt.bar(labels, values)
    plt.xticks(rotation=45)
    plt.ylabel("Standard Deviation (ms)")
    plt.title("Temporal variability per method (STD of P90)")
    plt.grid(axis="y", linestyle="--", alpha=0.5)
    plt.tight_layout()
    plt.show()
if SHOW_SPEEDUP_THREADS:
    m = ask_choice("matrix (id)", MATRICES)
    chunk = ask_choice("chunk size", DEFAULT_CHUNKS)
    plot_speedup_vs_threads(m, chunk)
elif SHOW_EFF_THREADS:
    m = ask_choice("matrix (id)", MATRICES)
    chunk = ask_choice("chunk size", DEFAULT_CHUNKS)
    plot_efficiency_vs_threads(m, chunk)
elif SHOW_TIME_THREADS:
    m = ask_choice("matrix (id)", MATRICES)
    chunk = ask_choice("chunk size", DEFAULT_CHUNKS)
    plot_time_vs_threads(m, chunk)
elif SHOW_ROOFLINE:
    m = ask_choice("matrix (id)", MATRICES)
    chunk = ask_choice("chunk size", DEFAULT_CHUNKS)
    
    sched_to_plot = ask_choice("scheduler (ej. guided_imp, dynamic)", schedules) 
    
    plot_roofline(m, chunk, sched_to_plot)
elif SHOW_SER_COMPAR:
    m = ask_choice("matrix (id)", MATRICES)

    ser_time = ser.get(m, np.nan)
    ser_imp_time = ser_imp.get(m, np.nan)

    plt.figure(figsize=(6,5))
    plt.bar(["SERIAL", "SERIAL_IMP"], [ser_time, ser_imp_time], color=["skyblue", "orange"])
    plt.ylabel("P90 Time (ms)")
    plt.title(f"P90 Time — Matrix {m}")
    if not np.isnan(ser_time) and not np.isnan(ser_imp_time):
        plt.text(0, ser_time + 0.01*max(ser_time, ser_imp_time), f"{ser_time:.2f} ms", ha="center", va="bottom")
        plt.text(1, ser_imp_time + 0.01*max(ser_time, ser_imp_time), f"{ser_imp_time:.2f} ms", ha="center", va="bottom")
    plt.grid(axis="y", linestyle="--", alpha=0.5)
    plt.tight_layout()
    plt.show()


# End of script