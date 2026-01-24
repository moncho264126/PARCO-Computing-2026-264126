import numpy as np
import matplotlib.pyplot as plt

# --- IMPORTS ---
from parser_weak import parse_weak_scaling
from parser_weak_concurrent import parse_weak_concurrent
from parser_weak_mpiio import parse_weak_mpiio
from parser_weak_2d import parse_weak_2d

# --- WEAK SCALING FLOPS CONFIG ---
# Parámetros usados en tu generación de matrices sintéticas
ROWS_PER_PROC = 50_000
NNZ_PER_ROW   = 50

NNZ_LOCAL   = ROWS_PER_PROC * NNZ_PER_ROW
FLOPS_LOCAL = 2 * NNZ_LOCAL  # FLOPs fijos POR proceso

def get_metrics(data_dict):
    if not data_dict: return None, None
    procs = np.array(sorted(data_dict.keys()))
    total = np.array([data_dict[p]["total"] for p in procs])
    return procs, total

def main():
    strategies = {
        "Centralized": parse_weak_scaling(),
        "Concurrent":  parse_weak_concurrent(),
        "MPI-IO":      parse_weak_mpiio(),
        "2D Grid":     parse_weak_2d()
    }
    
    styles = {
        "Centralized": {"c": "red",    "m": "o"},
        "Concurrent":  {"c": "blue",   "m": "s"},
        "MPI-IO":      {"c": "green",  "m": "^"},
        "2D Grid":     {"c": "purple", "m": "D"}
    }

    print("\n" + "="*80)
    print(f"{'WEAK SCALING RESULTS SUMMARY (With FLOPs)':^80}")
    print("="*80)

    for name, data in strategies.items():
        procs, total = get_metrics(data)
        if procs is None:
            print(f"\n>> Strategy: {name} (NO DATA)")
            continue
        
        # Weak Efficiency = T_base / T_current
        efficiency = total[0] / total 

        # --- FLOPs metrics (weak scaling) ---
        # Trabajo total = (FLOPs por proceso) * (Num Procesos)
        flops_total_work = FLOPS_LOCAL * procs 
        
        # GFLOPs/s Globales del sistema
        gflops = flops_total_work / total / 1e9
        
        # GFLOPs/s Por Proceso (Debería ser constante idealmente)
        gflops_per_proc = FLOPS_LOCAL / total / 1e9

        print(f"\n>> Strategy: {name}")
        # Header actualizado
        print(f"{'NP':<4} {'Time[s]':>10} {'Eff (T1/Tp)':>12} "
              f"{'GFLOPs/s':>10} {'GFLOPs/s/P':>13}")
        print("-" * 65)
        for i, p in enumerate(procs):
            print(f"{p:<4} {total[i]:10.4f} {efficiency[i]:12.2f} "
                  f"{gflops[i]:10.2f} {gflops_per_proc[i]:13.4f}")

    # --- PLOTS ---

    # FIG 1: Absolute Time
    plt.figure(figsize=(10, 6))
    for name, data in strategies.items():
        procs, total = get_metrics(data)
        if procs is None: continue
        plt.plot(procs, total, label=name, color=styles[name]["c"], marker=styles[name]["m"])
    
    plt.xscale("log", base=2)
    plt.xlabel("MPI Processes")
    plt.ylabel("Execution Time [s]")
    plt.title("Weak Scaling: Time to Solution (Ideal = Flat)")
    plt.grid(True, which="both", alpha=0.3)
    plt.legend()
    print("\nDisplaying Plot 1: Time...")
    plt.show()

    # FIG 2: Efficiency
    plt.figure(figsize=(10, 6))
    for name, data in strategies.items():
        procs, total = get_metrics(data)
        if procs is None: continue
        efficiency = total[0] / total
        plt.plot(procs, efficiency, label=name, color=styles[name]["c"], marker=styles[name]["m"])

    plt.axhline(1.0, color='k', linestyle='--', label="Ideal")
    plt.xscale("log", base=2)
    plt.ylim(0, 1.2)
    plt.xlabel("MPI Processes")
    plt.ylabel("Weak Efficiency (T_base / T_p)")
    plt.title("Weak Scaling Efficiency")
    plt.grid(True, which="both", alpha=0.3)
    plt.legend()
    print("Displaying Plot 2: Efficiency...")
    plt.show()

if __name__ == "__main__":
    main()