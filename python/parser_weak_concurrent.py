import os
import numpy as np

RESULTS_DIR = "../results"
NP_LIST = [1, 2, 4, 8, 16, 32, 64, 128]

def extract_times(filepath):
    total, comm, comp = [], [], []

    with open(filepath, "r") as f:
        for line in f:
            if "|" not in line:
                continue
            parts = [p.strip() for p in line.split("|")]
            if len(parts) != 4:
                continue
            try:
                total.append(float(parts[1]))
                comm.append(float(parts[2]))
                comp.append(float(parts[3]))
            except ValueError:
                continue

    return np.array(total), np.array(comm), np.array(comp)

def p90(arr):
    return np.percentile(arr, 90) if len(arr) > 0 else None

def parse_weak_concurrent():
    results = {}

    for np_val in NP_LIST:
        fname = f"weak_concurrent_np_{np_val}.txt"
        path = os.path.join(RESULTS_DIR, fname)

        if not os.path.isfile(path):
            print(f"WARNING: {fname} not found")
            continue

        total, comm, comp = extract_times(path)
        if len(total) == 0:
            continue

        results[np_val] = {
            "total": p90(total),
            "comm":  p90(comm),
            "comp":  p90(comp),
        }

    return results
