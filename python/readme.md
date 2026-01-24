# Distributed SpMV Performance Analysis

## Description
This project contains a suite of Python scripts designed to parse, analyze, and visualize the performance results of the **Distributed Sparse Matrix-Vector Multiplication (SpMV)** experiments.

Unlike the previous shared-memory implementation, these scripts focus on analyzing **MPI-based distributed strategies** across two scaling regimes:
1.  **Strong Scaling:** Fixed global problem size as process count ($P$) increases.
2.  **Weak Scaling:** Fixed problem size per process as $P$ increases.

The suite compares four distinct strategies:
* **Baseline (1D):** Centralized input with `MPI_Allgather`.
* **Concurrent (1D):** Independent file loading per process.
* **MPI-IO (1D):** Collective parallel I/O.
* **2D Grid:** Two-dimensional partitioning with row/col communicators.

## Script Categories

The scripts are organized by function and scaling regime:

### 1. Comparative Drivers (Main Scripts)
These are the primary scripts you should run to generate the final plots for the report. They aggregate data from all strategies to produce comparative graphs.
* `compare_strong.py`: Generates execution time, speedup, and efficiency plots for Strong Scaling.
* `compare_weak.py`: Generates execution time stability and aggregate GFLOPs plots for Weak Scaling.

### 2. Strategy-Specific Analyzers
These scripts analyze a single strategy in isolation. Useful for debugging specific data points.
* `analyze_strong.py` / `analyze_weak.py`: Analyzes the **Baseline (Centralized)** strategy.
* `analyze_strong_concurrent.py` / ... : Analyzes the **Concurrent I/O** strategy.
* `analyze_strong_mpiio.py` / ... : Analyzes the **MPI-IO** strategy.
* `analyze_strong_2d.py` / ... : Analyzes the **2D Grid** strategy.

### 3. Parsers
Backend scripts responsible for reading the raw `.txt` log files from the `../results/` directory and extracting P90 execution times and breakdown metrics (Compute vs. Communicate).
* `parser_*.py`: Matches the naming convention of the analyzers.

## Features

- **Automated Parsing:** Reads P90 values from raw text logs in `../results`.
- **Metric Calculation:**
    - **Strong Scaling:** Time ($s$), Speedup ($S = T_1 / T_N$), Parallel Efficiency ($E = S / N$).
    - **Weak Scaling:** Weak Efficiency, Aggregate GFLOPs/s.
- **Visualization:** Uses `matplotlib` to generate publication-ready figures:
    - `time_strong.png`, `speedup_efficiency.png`
    - `time_weak.png`, `weak_eff.png`

## Requirements
- Python 3.x
- `matplotlib`
- `numpy`

## Usage

### To Generate Comparison Plots (Recommended)
Run the comparison scripts to verify the performance differences between 1D and 2D strategies:

```bash
# For Strong Scaling (Speedup & Time)
python compare_strong.py

# For Weak Scaling (GFLOPs & Stability)
python compare_weak.py