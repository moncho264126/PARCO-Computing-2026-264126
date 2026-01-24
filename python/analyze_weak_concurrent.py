import numpy as np
import matplotlib.pyplot as plt
from parser_weak_concurrent import parse_weak_concurrent

data = parse_weak_concurrent()

np_vals = np.array(sorted(data.keys()))
total = np.array([data[k]["total"] for k in np_vals])
comm  = np.array([data[k]["comm"]  for k in np_vals])
comp  = np.array([data[k]["comp"]  for k in np_vals])

norm = total / total[0]

print("\nWeak Scaling – Concurrent (P90)")
print("NP   Total[s]   Comm[s]    Comp[s]    Norm")
for p, t, c, k, n in zip(np_vals, total, comm, comp, norm):
    print(f"{p:<4} {t:9.3e} {c:9.3e} {k:9.3e} {n:7.2f}")

# --- Total time ---
plt.figure()
plt.plot(np_vals, total, marker='o')
plt.xscale("log", base=2)
plt.xlabel("MPI processes")
plt.ylabel("P90 Time [s]")
plt.title("Weak Scaling – Concurrent")
plt.grid(True, which="both")
plt.tight_layout()
plt.show()

# --- Normalized ---
plt.figure()
plt.plot(np_vals, norm, marker='o')
plt.axhline(1.0, linestyle='--', label="Ideal")
plt.xscale("log", base=2)
plt.xlabel("MPI processes")
plt.ylabel("Normalized P90 Time")
plt.title("Weak Scaling Efficiency – Concurrent")
plt.legend()
plt.grid(True, which="both")
plt.tight_layout()
plt.show()
