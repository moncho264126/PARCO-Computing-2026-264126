# Simulation Scripts Overview

This folder contains all the scripts used to run performance and timing measurements for the Sparse Matrix-Vector multiplication programs.

## Folder Structure

- **perf_**  
  Contains shell scripts (`.sh`) designed to collect **hardware performance counters** using `perf`.

- **run_**  
  Contains shell scripts (`.sh`) to measure **execution time** of the programs. They run the executables multiple times to collect elapsed time statistics.

## Notes 
- Ensure that all required executables are compiled and available before running the scripts.  