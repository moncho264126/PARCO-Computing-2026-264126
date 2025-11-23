# Performance Analysis of Parallel Scheduling Methods

## Description
This project provides Python scripts to analyze and visualize the performance of multiple scheduling strategies applied to sparse matrix-vector multiplications (SpMV) with varying matrix sizes and thread counts. Both standard and implicitly optimized versions of the methods are included.

There are two main types of analyses:

1. **Execution time analysis (`analyze_performance.py`)**  
   - Computes P90 execution times from result files (`../results/*`).  
   - Supports serial (`SERIAL`, `SERIAL_IMP`) and parallel schedules (`auto`, `static`, `guided`, `dynamic`, `runtime`), with and without implicit optimizations.  
   - Generates visualizations such as bar charts, heatmaps, speedup, efficiency, GFLOPS, Roofline models, and P90 time vs threads.  
   - Computes rankings of methods based on average P90 times.  
   - Interactive menu allows selective plotting and queries.

2. **Hardware performance counters analysis (`parser_perf.py` + `results_perf` scripts)**  
   - `parser_perf.py` parses perf counter logs from `../results_perf/*`.  
   - Extracts counters like `L1_loads`, `L1_misses`, `LLC_loads`, `LLC_misses`.  
   - Supports serial, implicitly optimized serial, and parallel schedules, with threads, matrix, and chunk-size hierarchy.  
   - Interactive plotting scripts visualize P90 counter values vs threads, with markers per schedule and printed gains relative to `SERIAL` and `SERIAL_IMP`.

## Features
- Accumulates timing data across all matrices, as well as subsets of small (1 & 2) and large (3,4,5) matrices.
- Interactive and customizable plots for:
  - Base bar charts comparing all methods per matrix, thread count, and chunk size
  - Heatmaps of average P90 times by schedule
  - Average speedup and efficiency relative to `SERIAL` and `SERIAL_IMP`
  - Variability (standard deviation) per method
  - GFLOPS and Roofline analysis (requires original `.mtx` matrices)
  - P90 time, speedup, and efficiency vs threads per matrix
- Computes method rankings based on average P90 times.
- Performance counters analysis includes P90 plotting, markers per schedule, and printed gains relative to baseline serial runs.

## Requirements
- Python 3.x
- matplotlib
- numpy
- seaborn (optional, for heatmaps)
- `parser.py` (for execution time analysis)
- `parser_perf.py` (for perf counter analysis)

## Usage

1. Clone the repository or download the scripts.

2. Make sure all dependencies are installed:
   pip install matplotlib numpy seaborn


3. For execution time analysis (analyze_performance.py):
   python analyze_performance.py
- The interactive menu lets you choose which plots to display: bar charts, heatmaps, speedup, efficiency, GFLOPS, and Roofline.

4. For hardware performance counters analysis (plot_perf_counters.py):
   python plot_perf_counters.py
- The script will ask for a matrix number and a chunksize.
- Displays P90 counter values versus threads with different markers per schedule.
- Prints speedup relative to SERIAL and SERIAL_IMP.