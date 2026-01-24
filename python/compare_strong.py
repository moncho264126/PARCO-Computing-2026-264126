import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

# --- IMPORTS DE TUS PARSERS ---
from parser_strong import parse_strong_scaling          # Centralized
from parser_strong_concurrent import parse_strong_concurrent
from parser_strong_mpiio import parse_strong_mpiio
from parser_strong_2d import parse_strong_2d

# --- STRONG SCALING FLOPS CONFIG ---
# Ajustado a tu matriz real
NNZ_TOTAL = 270_234_840
FLOPS_TOTAL = 2 * NNZ_TOTAL

def get_metrics(data_dict):
    if not data_dict: return None, None, None, None
    procs = np.array(sorted(data_dict.keys()))
    total = np.array([data_dict[p]["total"] for p in procs])
    comm  = np.array([data_dict[p]["comm"]  for p in procs])
    comp  = np.array([data_dict[p]["comp"]  for p in procs])
    return procs, total, comm, comp

def main():
    # 1. Cargar Datos
    strategies = {
        "Centralized": parse_strong_scaling(),
        "Concurrent":  parse_strong_concurrent(),
        "MPI-IO":      parse_strong_mpiio(),
        "2D Grid":     parse_strong_2d()
    }

    # Estilos consistentes
    styles = {
        "Centralized": {"c": "red",    "m": "o", "ls": "-"},
        "Concurrent":  {"c": "blue",   "m": "s", "ls": "--"},
        "MPI-IO":      {"c": "green",  "m": "^", "ls": "-."},
        "2D Grid":     {"c": "purple", "m": "D", "ls": ":"}
    }

    # --- PART 1: PRINT TABLES IN TERMINAL ---
    print("\n" + "="*95)
    print(f"{'STRONG SCALING RESULTS SUMMARY (With FLOPs)':^95}")
    print("="*95)

    for name, data in strategies.items():
        procs, total, comm, comp = get_metrics(data)
        if procs is None:
            print(f"\n>> Strategy: {name} (NO DATA)")
            continue

        t1 = total[0]
        speedup = t1 / total
        efficiency = speedup / (procs / procs[0]) # Normalizado al primer rank
        comm_pct = (comm / total) * 100

        # --- FLOPs metrics ---
        # GFLOPs totales = (Operaciones totales) / Tiempo / 1e9
        gflops = FLOPS_TOTAL / total / 1e9
        # GFLOPs por proceso = GFLOPs totales / N_procs
        gflops_per_proc = gflops / procs

        print(f"\n>> Strategy: {name}")
        # Header ampliado
        print(f"{'NP':<4} {'Time[s]':>10} {'Speedup':>8} {'Eff':>6} {'Comm %':>8} "
              f"{'Comp[s]':>10} {'GFLOPs/s':>10} {'GFLOPs/s/P':>13}")
        print("-" * 80)
        
        for i, p in enumerate(procs):
            print(f"{p:<4} {total[i]:10.4f} {speedup[i]:8.2f} {efficiency[i]:6.2f} "
                  f"{comm_pct[i]:8.1f}% {comp[i]:10.4f} {gflops[i]:10.2f} {gflops_per_proc[i]:13.4f}")

    # --- PART 2: PLOTS (Show on Screen) ---

    # FIG 1: Time Comparison
    plt.figure(figsize=(10, 6))
    for name, data in strategies.items():
        procs, total, _, _ = get_metrics(data)
        if procs is None: continue
        plt.plot(procs, total, label=name, color=styles[name]["c"], marker=styles[name]["m"])
    
    plt.xscale("log", base=2)
    plt.yscale("log")
    plt.xlabel("MPI Processes")
    plt.ylabel("Execution Time [s]")
    plt.title("Strong Scaling: Absolute Execution Time")
    plt.grid(True, which="both", alpha=0.3)
    plt.legend()
    plt.tight_layout()
    print("\nDisplaying Plot 1: Execution Time...")
    plt.show()

    # FIG 2: Speedup & Efficiency
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    for name, data in strategies.items():
        procs, total, _, _ = get_metrics(data)
        if procs is None: continue
        t1 = total[0]
        speedup = t1 / total
        eff = speedup / (procs/procs[0])
        
        ax1.plot(procs, speedup, label=name, color=styles[name]["c"], marker=styles[name]["m"])
        ax2.plot(procs, eff, label=name, color=styles[name]["c"], marker=styles[name]["m"])

    # Ideal lines
    all_procs = sorted(list(set([p for d in strategies.values() if d for p in d])))
    if all_procs:
        p_arr = np.array(all_procs)
        ax1.plot(p_arr, p_arr/p_arr[0], 'k--', alpha=0.4, label="Ideal")
        ax2.axhline(1.0, color='k', linestyle='--', alpha=0.4, label="Ideal")

    ax1.set_title("Speedup"); ax1.set_xscale("log", base=2); ax1.set_yscale("log", base=2)
    ax1.legend(); ax1.grid(True, alpha=0.3)
    ax2.set_title("Efficiency"); ax2.set_xscale("log", base=2); ax2.set_ylim(0, 1.2)
    ax2.legend(); ax2.grid(True, alpha=0.3)
    plt.tight_layout()
    print("Displaying Plot 2: Speedup & Efficiency...")
    plt.show()

    # FIG 3: Comm vs Comp Breakdown (Stacked Bars)
    fig, ax = plt.subplots(figsize=(12, 6))
    target_procs = [16, 32, 64, 128] # Ajusta según tus datos
    indices = np.arange(len(target_procs))
    width = 0.2
    offset = -0.3

    for name, data in strategies.items():
        if not data: continue
        comm_ratios = []
        for p in target_procs:
            if p in data:
                t = data[p]["total"]
                c = data[p]["comm"]
                comm_ratios.append((c/t)*100)
            else:
                comm_ratios.append(0)
        
        ax.bar(indices + offset, comm_ratios, width, label=name, color=styles[name]["c"], alpha=0.8)
        offset += width

    ax.set_ylabel("% Time in Communication")
    ax.set_title("Communication Overhead Comparison")
    ax.set_xticks(indices + 0.1)
    ax.set_xticklabels([f"P={p}" for p in target_procs])
    ax.set_ylim(0, 100)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    print("Displaying Plot 3: Communication Breakdown...")
    plt.show()

if __name__ == "__main__":
    main()