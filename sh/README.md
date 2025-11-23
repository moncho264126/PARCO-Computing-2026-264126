# Simulation Scripts Overview

This folder contains all the scripts used to run performance and timing measurements for the Sparse Matrix-Vector multiplication programs.

## Folder Structure

- **perf/**  
  Contains shell scripts (`.sh`) designed to collect **hardware performance counters** using `perf`.

- **time/**  
  Contains shell scripts (`.sh`) to measure **execution time** of the programs. They run the executables multiple times to collect elapsed time statistics.

## Notes

- Each `.sh` script is **well-documented with descriptive comments**, explaining its purpose, input files, and execution procedure.  
- Ensure that all required executables are compiled and available before running the scripts.  
- The structure allows you to separate **performance measurements** (`perf/`) from **timing measurements** (`time/`) for clarity and reproducibility.