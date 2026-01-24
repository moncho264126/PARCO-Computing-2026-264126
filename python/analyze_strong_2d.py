import numpy as np
import matplotlib.pyplot as plt
from parser_strong_2d import parse_strong_2d

def main():
    data = parse_strong_2d()

    if not data:
        print("No Data Found for Strong 2D Scaling.")
        return

    np_vals = np.array(sorted(data.keys()))
    total = np.array([data[k]["total"] for k in np_vals])
    comm  = np.array([data[k]["comm"]  for k in np_vals])
    comp  = np.array([data[k]["comp"]  for k in np_vals])

    # --- Métricas Strong Scaling ---
    # T1 es el tiempo base (con 1 proceso)
    t1 = total[0]
    
    # Speedup: Cuántas veces más rápido es con N procesos
    speedup = t1 / total
    
    # Eficiencia: Qué tan bien aprovechamos los recursos (ideal = 1.0)
    efficiency = speedup / np_vals

    # --- Tabla en Terminal ---
    print("\nStrong Scaling – 2D Grid (P90)")
    print(f"{'NP':<4} {'Total[s]':>9} {'Comm[s]':>9} {'Comp[s]':>9} {'Speedup':>8} {'Eff':>6}")
    print("-" * 55)
    for p, t, c, k, s, e in zip(np_vals, total, comm, comp, speedup, efficiency):
        print(f"{p:<4} {t:9.3e} {c:9.3e} {k:9.3e} {s:8.2f} {e:6.2f}")

    # --- Plot 1: Desglose de Tiempos (Log-Log) ---
    plt.figure(figsize=(10, 6))
    plt.plot(np_vals, total, marker='o', linewidth=2, label="Total Time")
    plt.plot(np_vals, comm,  marker='^', linestyle=':', label="Communication")
    plt.plot(np_vals, comp,  marker='s', linestyle='--', label="Computation")

    # Línea ideal (referencia visual para T1/N)
    ideal_time = t1 / np_vals
    plt.plot(np_vals, ideal_time, 'k--', alpha=0.6, label="Ideal Scaling (1/N)")

    plt.xscale("log", base=2)
    plt.yscale("log")
    plt.xlabel("MPI processes")
    plt.ylabel("Time [s]")
    plt.title("Strong Scaling 2D – Time Breakdown")
    plt.grid(True, which="both", alpha=0.3)
    plt.legend()
    plt.xticks(np_vals, labels=[str(n) for n in np_vals])
    
    plt.tight_layout()
    plt.show()

    # --- Plot 2: Speedup ---
    plt.figure(figsize=(10, 6))
    plt.plot(np_vals, speedup, marker='o', color='green', linewidth=2, label="Measured Speedup 2D")
    
    # Línea ideal de Speedup (y = x)
    plt.plot(np_vals, np_vals, 'k--', alpha=0.6, label="Ideal Linear Speedup")
    
    plt.xscale("log", base=2)
    plt.yscale("log", base=2)
    plt.xlabel("MPI processes")
    plt.ylabel("Speedup (T1 / Tn)")
    plt.title("Strong Scaling 2D – Speedup")
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    plt.xticks(np_vals, labels=[str(n) for n in np_vals])
    plt.yticks(np_vals, labels=[str(n) for n in np_vals])
    
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()