# PBS Job Scripts for Performance and Timing Simulations

## Overview

This folder contains PBS job scripts used to run simulations for performance (`perf`) and timing (`time`) measurements. Each script corresponds to a specific scheduling method or configuration.

The folder structure is:

```
perf/           # Shell scripts for hardware performance counter measurements
time/           # Shell scripts for execution time measurements
```

## Example PBS Script

```bash
#!/bin/bash
#PBS -N par_dynamic_Job16
#PBS -q short_cpuQ
#PBS -l select=1:ncpus=32:mem=50gb
#PBS -l walltime=06:00:00
#PBS -o ../logs/parallel_perf_dynamic_output.log
#PBS -e ../logs/parallel_perf_dynamic_error.log

# Load modules
module purge
module load perf
module load gcc91

# Submission directory
cd $PBS_O_WORKDIR

# Create log directory
mkdir -p ../logs

# Ensure that the execution script is executable
chmod +x ../sh/perf/perf_par_dynamic.sh

# Execute
../sh/perf/perf_par_dynamic.sh
```

## Key Points

* `#PBS -N` — Job name.
* `#PBS -q` — Queue to submit the job.
* `#PBS -l select` — Number of nodes, CPUs, and memory requested.
* `#PBS -l walltime` — Maximum allowed execution time.
* `#PBS -o` / `#PBS -e` — Standard output and error logs.
* Modules like `perf` and `gcc91` are loaded for the execution environment.
* Scripts are executed from the working directory and write logs to `logs/`.
* The actual simulation is executed by the corresponding shell script inside `sh/`.

## Usage

1. Place yourself in the folder of the scrip you want to submit
2. Submit the job to the cluster using:

   ```bash
   qsub <pbs_script_name>.pbs
   ```
3. The PBS script will handle module loading, setup, and execution of the corresponding shell script in `perf/` or `time/`.
4. Logs for the job execution are saved in `logs/` as specified in the PBS directives.
5. Results will be saved in `results/` and `results_perf/` in the repo directory