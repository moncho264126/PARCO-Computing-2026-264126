import numpy as np
import matplotlib.pyplot as plt
from parser_weak import parse_weak_scaling

data = parse_weak_scaling()

procs = np.array(list(data.keys()))
total = np.array([data[p]["total"] for p in procs])
comm  = np.array([data[p]["comm"]  for p in procs])
comp  = np.array([data[p]["comp"]  for p in procs])

norm_total = total / total[0]

print("\nWeak Scaling (P90)")
print("NP   Total     Comm      Comp      Norm")
for p, t, c, k, n in zip(procs, total, comm, comp, norm_total):
    print(f"{p:<4} {t:.3e} {c:.3e} {k:.3e} {n:5.2f}")

plt.figure()
plt.plot(procs, total, label="Total")
plt.plot(procs, comm, label="Communication")
plt.plot(procs, comp, label="Computation")
plt.xscale("log", base=2)
plt.xlabel("MPI Processes")
plt.ylabel("Time [s]")
plt.title("Weak Scaling Breakdown (P90)")
plt.legend()
plt.grid(True, which="both")
plt.tight_layout()
plt.show()