#!/usr/bin/env python3
"""
===============================================================================
                PARSER FOR PERFORMANCE COUNTER LOGS
===============================================================================

Script: parser_perf.py
Description:
    Parses hardware performance counter logs from SpMV simulations stored in
    ../results_perf/* directories. Extracts L1/L3 cache loads and misses for
    serial and parallel runs with various scheduling strategies.

Directory structure expected:

results_perf/
 ├── ser/
 │     ser_1.txt
 │     ser_2.txt
 ├── ser_imp/
 │     ser_imp_1.txt
 ├── par_auto/
 │     par_auto_<threads>_<matrix>.txt
 ├── par_static/
 │     par_static_<threads>_<chunk>_<matrix>.txt
 ├── par_dynamic/
 │     ...
 └── ...

Returned data structures:
    ser[d] = counters_dict
    ser_imp[d] = counters_dict
    par[schedule][threads][matrix][chunk?] = counters_dict

Where counters_dict contains numpy arrays:
    "L1_loads", "L1_misses", "LLC_loads", "LLC_misses"

Supported schedules:
    auto, auto_imp, static, static_imp,
    guided, guided_imp, dynamic, dynamic_imp,
    runtime, runtime_imp

Main Features:
1. Automatically iterates over all subfolders defined in DIRS.
2. Extracts counters line by line from each .txt file.
3. Supports both serial and parallel runs with or without chunks.
4. Gracefully handles missing files or folders, printing warnings.

Usage:
    from parser_perf import parse_all_results
    ser, ser_imp, par = parse_all_results()

Dependencies:
    - Python 3.x
    - numpy
    - os, re
===============================================================================
"""

import os
import re
import numpy as np

# ------------------------------------------------------------
# BASE DIRECTORY (one level above python/)
# ------------------------------------------------------------
BASE = os.path.join("..", "results_perf")

DIRS = {
    "ser":         os.path.join(BASE, "ser"),
    "ser_imp":     os.path.join(BASE, "ser_imp"),
    "auto":        os.path.join(BASE, "par_auto"),
    "auto_imp":    os.path.join(BASE, "par_auto_imp"),
    "static":      os.path.join(BASE, "par_static"),
    "static_imp":  os.path.join(BASE, "par_static_imp"),
    "guided":      os.path.join(BASE, "par_guided"),
    "guided_imp":  os.path.join(BASE, "par_guided_imp"),
    "dynamic":     os.path.join(BASE, "par_dynamic"),
    "dynamic_imp": os.path.join(BASE, "par_dynamic_imp"),
    "runtime":     os.path.join(BASE, "par_runtime"),
    "runtime_imp": os.path.join(BASE, "par_runtime_imp"),
}


# ------------------------------------------------------------
# Extract counters line by line
# Format example:
# 56,163,193,401 | 979,724,550 | 463,509,124 | 24,680,645
# ------------------------------------------------------------
line_re = re.compile(
    r"^\s*([\d,]+)\s*\|\s*([\d,]+)\s*\|\s*([\d,]+)\s*\|\s*([\d,]+)"
)

def extract_counters(path):
    """Reads all counter lines and returns dict of numpy arrays."""

    L1_loads = []
    L1_misses = []
    LLC_loads = []
    LLC_misses = []

    if not os.path.exists(path):
        return None

    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            m = line_re.match(line)
            if m:
                L1_loads.append(int(m.group(1).replace(",", "")))
                L1_misses.append(int(m.group(2).replace(",", "")))
                LLC_loads.append(int(m.group(3).replace(",", "")))
                LLC_misses.append(int(m.group(4).replace(",", "")))

    if not L1_loads:
        return None

    return {
        "L1_loads": np.array(L1_loads),
        "L1_misses": np.array(L1_misses),
        "LLC_loads": np.array(LLC_loads),
        "LLC_misses": np.array(LLC_misses),
    }


# ------------------------------------------------------------
# MAIN PARSER
# ------------------------------------------------------------
def parse_all_results():

    ser = {}
    ser_imp = {}

    par = {
        "auto": {},   "auto_imp": {},
        "static": {}, "static_imp": {},
        "guided": {}, "guided_imp": {},
        "dynamic": {}, "dynamic_imp": {},
        "runtime": {}, "runtime_imp": {},
    }

    # ------------------------------------
    # Walk through each folder
    # ------------------------------------
    for sched, folder in DIRS.items():

        if not os.path.isdir(folder):
            print(f"WARNING: folder not found: {folder}")
            continue

        for fname in os.listdir(folder):
            full = os.path.join(folder, fname)

            counters = extract_counters(full)
            if counters is None:
                continue

            # -----------------------------
            # SERIAL: ser_1.txt
            # -----------------------------
            if sched == "ser":
                m = re.match(r"ser_(\d+)\.txt", fname)
                if m:
                    ser[int(m.group(1))] = counters
                continue

            if sched == "ser_imp":
                m = re.match(r"ser_imp_(\d+)\.txt", fname)
                if m:
                    ser_imp[int(m.group(1))] = counters
                continue

            # -----------------------------
            # AUTO: par_auto_16_4.txt
            # (threads, matrix)
            # -----------------------------
            if sched in ["auto", "auto_imp"]:
                m = re.match(rf"par_{sched}_(\d+)_(\d+)\.txt", fname)
                if m:
                    t = int(m.group(1))
                    d = int(m.group(2))
                    par[sched].setdefault(t, {})[d] = counters
                continue

            # -----------------------------
            # SCHEDULE WITH CHUNK:
            # par_static_8_100_3.txt
            #  → threads, chunk, matrix
            # -----------------------------
            m = re.match(rf"par_{sched}_(\d+)_(\d+)_(\d+)\.txt", fname)
            if m:
                t = int(m.group(1))
                c = int(m.group(2))
                d = int(m.group(3))
                par[sched].setdefault(t, {}).setdefault(d, {})[c] = counters
                continue

    return ser, ser_imp, par



# ------------------------------------------------------------
# Quick test
# ------------------------------------------------------------
if __name__ == "__main__":
    ser, ser_imp, par = parse_all_results()
    print("Serial matrices found:", sorted(ser.keys()))
    print("Serial_imp matrices found:", sorted(ser_imp.keys()))

    for sched in par:
        print(f"{sched}: threads={sorted(par[sched].keys())}")
        