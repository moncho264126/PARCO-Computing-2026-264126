# Simulation Results

This folder contains the **results of all perf simulations** conducted with the Sparse Matrix-Vector multiplication programs.

## Folder Structure

The results are organized into subfolders based on the type of simulation:

- **ser/** – Results from the serial implementation.
- **ser_imp/** – Results from the serial implementation with optimizations.
- **par_guided_imp/** – Results from the parallel optimized implementation with OpenMP using guided scheduling.

Inside each of these subfolders, you will find **text files (`.txt`)** that store the output of the simulations.

## File Format

The naming convention of the result files follows the structure:

`par_{schedule}_{threads}_{chunksize}_{matrix}.txt`


Where:

- `<threads>` – Number of OpenMP threads used (only relevant for parallel implementations).
- `<chunksize>` – Chunk size used in scheduling (only relevant for parallel implementations).
- `<matrix>` – Identifier of the matrix used in the simulation.