import os
import numpy as np
import re

RESULTS_DIR = "../results"
# Patrón para strong scaling
FILE_PATTERN = re.compile(r"strong_mpiio_np_(\d+)\.txt")

def extract_times(filepath):
    total, comm, comp = [], [], []
    
    with open(filepath, "r") as f:
        for line in f:
            if "|" not in line or "Total_Time" in line:
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

def parse_strong_mpiio():
    results = {}
    
    if not os.path.exists(RESULTS_DIR):
        return {}

    for fname in os.listdir(RESULTS_DIR):
        m = FILE_PATTERN.match(fname)
        if not m:
            continue
        
        np_val = int(m.group(1))
        path = os.path.join(RESULTS_DIR, fname)
        
        total, comm, comp = extract_times(path)
        
        if len(total) == 0:
            continue
            
        results[np_val] = {
            "total": p90(total),
            "comm":  p90(comm),
            "comp":  p90(comp),
        }
        
    return dict(sorted(results.items()))

if __name__ == "__main__":
    print(parse_strong_mpiio())