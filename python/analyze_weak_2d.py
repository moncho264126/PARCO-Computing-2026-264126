import numpy as np
import matplotlib.pyplot as plt
from parser_weak_2d import parse_weak_2d

def main():
    data = parse_weak_2d()

    if not data:
        print("No Data Found for Weak 2D Scaling.")
        return

    np_vals = np.array(sorted(data.keys()))
    total = np.array([data[k]["total"] for k in np_vals])
    comm  = np.array([data[k]["comm"]  for k in np_vals])
    comp  = np.array([data[k]["comp"]  for k in np_vals])

    # --- Cálculo de Métricas ---
    # En Weak Scaling ideal, el tiempo se mantiene constante.
    # Normalizamos respecto al primer caso (NP=1).
    norm = total / total[0]

    # --- Print Table ---
    print("\nWeak Scaling 2D – Analysis (P90)")
    print(f"{'NP':<4} {'Total[s]':>10} {'Comm[s]':>10} {'Comp[s]':>10} {'Norm (T/T1)':>12}")
    print("-" * 55)
    for p, t, c, k, n in zip(np_vals, total, comm, comp, norm):
        print(f"{p:<4} {t:10.4e} {c:10.4e} {k:10.4e} {n:12.2f}")

    # --- Plot 1: Tiempo Absoluto ---
    plt.figure(figsize=(10, 6))
    plt.plot(np_vals, total, marker='o', linewidth=2, label='Total Time')
    plt.plot(np_vals, comp, marker='s', linestyle='--', label='Computation')
    plt.plot(np_vals, comm, marker='^', linestyle=':', label='Communication')
    
    plt.xscale("log", base=2)
    plt.xlabel("MPI processes")
    plt.ylabel("Time (s) [P90]")
    plt.title("Weak Scaling 2D – Absolute Time")
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    
    # Mostrar ticks de X como enteros (1, 2, 4...)
    plt.xticks(np_vals, labels=[str(n) for n in np_vals])
    
    plt.tight_layout()
    plt.show()

    # --- Plot 2: Eficiencia (Normalizado) ---
    plt.figure(figsize=(10, 6))
    plt.plot(np_vals, norm, marker='o', color='purple', linewidth=2, label="Measured 2D")
    plt.axhline(1.0, color='black', linestyle='--', label="Ideal Weak Scaling (1.0)")
    
    plt.xscale("log", base=2)
    plt.xlabel("MPI processes")
    plt.ylabel("Normalized Time (Lower is Better)")
    plt.title("Weak Scaling 2D – Efficiency")
    plt.legend()
    plt.grid(True, which="both", alpha=0.3)
    
    plt.xticks(np_vals, labels=[str(n) for n in np_vals])
    
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()