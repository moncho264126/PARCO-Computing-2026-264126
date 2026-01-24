import numpy as np
import matplotlib.pyplot as plt
from parser_strong import parse_strong_scaling

data = parse_strong_scaling()

procs = np.array(list(data.keys()))
total = np.array([data[p]["total"] for p in procs])
comm  = np.array([data[p]["comm"]  for p in procs])
comp  = np.array([data[p]["comp"]  for p in procs])

t1 = total[0]
speedup = t1 / total
efficiency = speedup / procs

print("\nStrong Scaling (P90)")
print("NP   Total     Comm      Comp      Speedup  Eff")
for p, t, c, k, s, e in zip(procs, total, comm, comp, speedup, efficiency):
    print(f"{p:<4} {t:.3e} {c:.3e} {k:.3e} {s:6.2f} {e:5.2f}")

plt.figure()
plt.plot(procs, total, label="Total")
plt.plot(procs, comm, label="Communication")
plt.plot(procs, comp, label="Computation")
plt.xscale("log", base=2)
plt.yscale("log")
plt.xlabel("MPI Processes")
plt.ylabel("Time [s]")
plt.title("Strong Scaling Breakdown (P90)")
plt.legend()
plt.grid(True, which="both")
plt.tight_layout()
plt.show()