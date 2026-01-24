import numpy as np
import matplotlib.pyplot as plt
from parser_strong_mpiio import parse_strong_mpiio

data = parse_strong_mpiio()

if not data:
    print("No Data Found for Strong MPI-IO")
    exit()

np_vals = np.array(sorted(data.keys()))
total = np.array([data[k]["total"] for k in np_vals])
comm  = np.array([data[k]["comm"]  for k in np_vals])
comp  = np.array([data[k]["comp"]  for k in np_vals])

# Calculo de Speedup y Eficiencia
t1 = total[0]
speedup = t1 / total
efficiency = speedup / np_vals

print("\nStrong Scaling – MPI-IO (P90)")
print("NP   Total[s]   Comm[s]    Comp[s]   Speedup   Eff")
for p, t, c, k, s, e in zip(np_vals, total, comm, comp, speedup, efficiency):
    print(f"{p:<4} {t:9.3e} {c:9.3e} {k:9.3e} {s:7.2f} {e:5.2f}")

# --- Plot 1: Desglose de Tiempos (Log-Log) ---
plt.figure(figsize=(10, 6))
plt.plot(np_vals, total, marker='o', label="Total Time")
plt.plot(np_vals, comm,  marker='^', label="Communication")
plt.plot(np_vals, comp,  marker='s', label="Computation")

# Línea ideal (referencia visual para T1/N)
ideal_time = t1 / np_vals
plt.plot(np_vals, ideal_time, 'k--', alpha=0.5, label="Ideal Scaling (1/N)")

plt.xscale("log", base=2)
plt.yscale("log")
plt.xlabel("MPI processes")
plt.ylabel("Time [s]")
plt.title("Strong Scaling – MPI-IO (Breakdown)")
plt.grid(True, which="both", alpha=0.3)
plt.legend()
plt.tight_layout()
plt.show()

# --- Plot 2: Speedup ---
plt.figure(figsize=(10, 6))
plt.plot(np_vals, speedup, marker='o', color='green', label="Measured Speedup")
plt.plot(np_vals, np_vals, 'k--', label="Ideal Speedup (Linear)")
plt.xscale("log", base=2)
plt.yscale("log", base=2)
plt.xlabel("MPI processes")
plt.ylabel("Speedup")
plt.title("Strong Scaling Speedup – MPI-IO")
plt.legend()
plt.grid(True, which="both", alpha=0.3)
plt.tight_layout()
plt.show()