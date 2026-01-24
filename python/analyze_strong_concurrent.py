import numpy as np
import matplotlib.pyplot as plt
from parser_strong_concurrent import parse_strong_concurrent

data = parse_strong_concurrent()

np_vals = np.array(list(data.keys()))
total = np.array([data[k]["total"] for k in np_vals])
comm  = np.array([data[k]["comm"]  for k in np_vals])
comp  = np.array([data[k]["comp"]  for k in np_vals])

t1 = total[0]
speedup = t1 / total
eff = speedup / np_vals

print("\nStrong Scaling – Concurrent (P90)")
print("NP   Total[s]   Comm[s]    Comp[s]   Speedup  Eff")
for p, t, c, k, s, e in zip(np_vals, total, comm, comp, speedup, eff):
    print(f"{p:<4} {t:9.3e} {c:9.3e} {k:9.3e} {s:7.2f} {e:5.2f}")

# --- Breakdown ---
plt.figure()
plt.plot(np_vals, total, marker='o', label="Total")
plt.plot(np_vals, comm,  marker='o', label="Communication")
plt.plot(np_vals, comp,  marker='o', label="Computation")
plt.xscale("log", base=2)
plt.yscale("log")
plt.xlabel("MPI processes")
plt.ylabel("Time [s]")
plt.title("Strong Scaling – Concurrent")
plt.grid(True, which="both")
plt.legend()
plt.tight_layout()
plt.show()