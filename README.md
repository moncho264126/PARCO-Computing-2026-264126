# Project Overview

This repository contains the complete setup for running and analyzing simulations of parallel matrix operations, including performance profiling and timing results.

## Directory Structure

```
matrix/          → Matrices and vectors used in simulations.
python/          → Python scripts for processing and visualizing results.
results/         → Output TXT files from timing simulations.
results_perf/    → Output TXT files from performance counter simulations.
sh/              → Shell scripts for compiling and executing simulations (loops, parameter settings).
src/             → Source code of the programs being simulated.
README.md        → This file.
```

## General Notes

* To **submit jobs**, submit the desired job script using `qsub`.
* The `sh/` folder contains scripts that compile and run the programs. If any simulation parameters change (matrix size, number of threads, chunk size, etc.), these scripts must be updated.
* All Python scripts in `python/` process the outputs stored in `results/` and `results_perf/`.
* Matrices and vectors for simulations are stored in the `matrix/` folder.

## Compiler Version and Flags

* Compiler versions and flags are specified in the PBS headers or in the `sh/` scripts.
* Example modules loaded in PBS scripts:

  ```bash
  module purge
  module load perf
  module load gcc91
  ```

## How to Compile and Run

### Local execution (optional, SEE PARTICULAR PROGRAM HEADER!!!):

```bash
# Navigate to src
cd src
# Compile (example)
gcc -g -Wall -fopenmp -o par_auto_spmv par_auto_spmv.c mmio.c
# Run locally with default parameters
./par_auto_spmv matrix.mtx vector.mtx <threads> <chunk_size>
```

### Cluster execution:

```bash
# Submit job
qsub run_ser_imp.sh
```

* The jobs automatically create a `logs/` folder in the submission directory to store output and error logs.

## Input and Output

* **Input**: matrices and vectors from `matrix/`, number of threads, chunk size, and schedule type defined in shell scripts.
* **Output**:

  * `results/` → Timing results per matrix, threads, chunk, and schedule (TXT files).
  * `results_perf/` → Hardware performance counters per matrix, threads, chunk, and schedule (TXT files).
* Python scripts process these outputs to generate metrics, plots, and rankings.

## Modifying Simulation Parameters

* Matrix sizes and vectors → edit files in `matrix/`.
* Number of threads/CPUs and chunk sizes → modify PBS headers or shell scripts in `sh/`.
* Schedule type → choose the appropriate PBS or shell script.

**Default Values:** typically 32 CPUs per job, 50 GB RAM, and walltime of 6 hours (see PBS headers).

## Cluster-Specific Notes

* **Modules loaded**: `perf`, `gcc91`.
* **Queue**: `short_cpuQ`.
* PBS scripts handle setting the working directory, creating `logs/`, and running the appropriate shell script automatically.

## Summary

This structure allows easy submission, automated compilation and execution, and straightforward collection and analysis of both timing and hardware performance results. Modifying parameters is centralized in PBS headers and shell scripts for consistency.