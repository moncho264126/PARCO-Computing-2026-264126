import numpy as np
import matplotlib.pyplot as plt
from parser_weak_mpiio import parse_weak_mpiio

data = parse_weak_mpiio()

if not data:
    print("No Data Found for Weak MPI-IO")
    exit()

np_vals = np.array(sorted(data.keys()))
total = np.array([data[k]["total"] for k in np_vals])
comm  = np.array([data[k]["comm"]  for k in np_vals])
comp  = np.array([data[k]["comp"]  for k in np_vals])

# En Weak Scaling, lo ideal es que el tiempo se mantenga constante (Ratio 1.0)
norm = total / total[0]

print("\nWeak Scaling – MPI-IO (P90)")
print("NP   Total[s]   Comm[s]    Comp[s]    Norm (T/T1)")
for p, t, c, k, n in zip(np_vals, total, comm, comp, norm):
    print(f"{p:<4} {t:9.3e} {c:9.3e} {k:9.3e} {n:7.2f}")

# --- Plot 1: Tiempo Absoluto ---
plt.figure(figsize=(10, 6))
plt.plot(np_vals, total, marker='o', label='Total Time')
plt.plot(np_vals, comp, marker='s', linestyle='--', label='Computation')
plt.plot(np_vals, comm, marker='^', linestyle=':', label='Communication')
plt.xscale("log", base=2)
plt.xlabel("MPI processes")
plt.ylabel("P90 Time [s]")
plt.title("Weak Scaling – MPI-IO (Time)")
plt.legend()
plt.grid(True, which="both", alpha=0.3)
plt.tight_layout()
plt.show()

# --- Plot 2: Eficiencia (Normalizado) ---
plt.figure(figsize=(10, 6))
plt.plot(np_vals, norm, marker='o', color='purple', label="Measured")
plt.axhline(1.0, color='black', linestyle='--', label="Ideal Weak Scaling")
plt.xscale("log", base=2)
plt.xlabel("MPI processes")
plt.ylabel("Normalized Time (Lower is Better)")
plt.title("Weak Scaling Efficiency – MPI-IO")
plt.legend()
plt.grid(True, which="both", alpha=0.3)
plt.tight_layout()
plt.show()