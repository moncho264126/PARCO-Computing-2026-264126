import os
import re
import numpy as np

RESULTS_DIR = "../results"
NP_LIST = [1, 2, 4, 8, 16, 32, 64, 128]

LINE_PATTERN = re.compile(
    r"\d+\s*\|\s*([0-9.eE+-]+)\s*\|\s*([0-9.eE+-]+)\s*\|\s*([0-9.eE+-]+)"
)

def parse_weak_scaling():
    """
    Returns:
        dict {
          NP: {
            "total": p90,
            "comm":  p90,
            "comp":  p90
          }
        }
    """
    results = {}

    for np_ in NP_LIST:
        fname = f"weak_np_{np_}.txt"
        path = os.path.join(RESULTS_DIR, fname)

        if not os.path.isfile(path):
            print(f"WARNING: {fname} not found")
            continue

        total, comm, comp = [], [], []

        with open(path, "r") as f:
            for line in f:
                m = LINE_PATTERN.search(line)
                if m:
                    total.append(float(m.group(1)))
                    comm.append(float(m.group(2)))
                    comp.append(float(m.group(3)))

        if total:
            results[np_] = {
                "total": np.percentile(total, 90),
                "comm":  np.percentile(comm, 90),
                "comp":  np.percentile(comp, 90),
            }

    return results