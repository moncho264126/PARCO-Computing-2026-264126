# Experimental Results (Distributed SpMV)

This folder contains the **raw output logs** from the distributed Sparse Matrix-Vector multiplication experiments using MPI.

## File Naming Convention

The results are stored in text files (`.txt`) following a strict naming convention that identifies the scaling regime, the I/O strategy, and the process count.

**Format:**
`[scaling]_[strategy]_np_[count].txt`

### Components:

1.  **Scaling Regime** (`[scaling]`):
    * `strong`: **Strong Scaling** (Fixed global problem size, measuring speedup).
    * `weak`: **Weak Scaling** (Fixed problem size per process, measuring throughput stability).

2.  **Strategy / Partitioning** (`[strategy]`):
    * `2d`: **2D Grid Partitioning** (Uses row/col communicators).
    * `mpiio`: **1D Partitioning with MPI-IO** (Parallel collective I/O).
    * `concurrent`: **1D Partitioning with Concurrent I/O** (Independent file access).
    * *(None)*: **1D Partitioning with Centralized Input** (Baseline strategy, e.g., `strong_np_128.txt`).

3.  **Process Count** (`[count]`):
    * The integer following `np_` represents the number of MPI processes used (e.g., `1`, `8`, `64`, `128`).

## Examples

| Filename | Description |
| :--- | :--- |
| `strong_2d_np_128.txt` | Strong scaling, 2D Grid strategy, 128 Processes. |
| `weak_mpiio_np_64.txt` | Weak scaling, 1D Partitioning (MPI-IO), 64 Processes. |
| `strong_np_32.txt` | Strong scaling, 1D Partitioning (Centralized Baseline), 32 Processes. |
| `weak_concurrent_np_16.txt` | Weak scaling, 1D Partitioning (Concurrent Read), 16 Processes. |