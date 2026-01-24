import os
import re
import numpy as np

RESULTS_DIR = "../results"

FILE_PATTERN = re.compile(r"strong_np_(\d+)\.txt")
LINE_PATTERN = re.compile(
    r"\d+\s*\|\s*([0-9.eE+-]+)\s*\|\s*([0-9.eE+-]+)\s*\|\s*([0-9.eE+-]+)"
)

def parse_strong_scaling():
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

    for fname in os.listdir(RESULTS_DIR):
        m = FILE_PATTERN.match(fname)
        if not m:
            continue

        np_proc = int(m.group(1))
        path = os.path.join(RESULTS_DIR, fname)

        total, comm, comp = [], [], []

        with open(path, "r") as f:
            for line in f:
                m = LINE_PATTERN.search(line)
                if m:
                    total.append(float(m.group(1)))
                    comm.append(float(m.group(2)))
                    comp.append(float(m.group(3)))

        if total:
            results[np_proc] = {
                "total": np.percentile(total, 90),
                "comm":  np.percentile(comm, 90),
                "comp":  np.percentile(comp, 90),
            }

    return dict(sorted(results.items()))