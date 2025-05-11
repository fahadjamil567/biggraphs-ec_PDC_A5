# BFS Parallelization Write-Up
**Partners**: Muhammad Taahaa 22L-6717, Muhammad Fahad Jamil 22i-1025

## Top-Down BFS (Part 1)
![Top Down Parallel Part 1](/output/Top_down_parallel_part1.png)

## Pre Part 1
![Pre Part 1](/output/prepart_1.png)

## Bottom-Up BFS (Part B)
![Bottom Up BFS Part B](/output/Bottom%20Up%20BFS_part%20B.png)

## Hybrid BFS Correctness (Part C)
![Hybrid Search Correctness](/output/Partc_HybridSearchCorrectness.png)

## Final Output Table
![Final Output](/output/Final_output.png)

---

## Implementation Overview

We implemented three versions of parallel BFS:
1. **Top-Down BFS**: Each node in the current frontier explores its neighbors. If a neighbor is unvisited, it gets added to the next frontier.
2. **Bottom-Up BFS**: All unvisited nodes check whether any of their incoming neighbors are in the current frontier. If so, they mark themselves visited and join the next frontier.
3. **Hybrid BFS**: Dynamically switches between top-down and bottom-up depending on frontier density and average out-degree.

All implementations use OpenMP to parallelize the iterations and atomic operations to avoid race conditions.

---

## Optimization Process

### Part 1: Top-Down BFS
**Synchronization**: Used `__sync_bool_compare_and_swap()` to atomically update the `distances` array and an `#pragma omp atomic` to safely increment the new frontier count.

**Overhead Mitigation**: Limited synchronization to only necessary updates. Also used `schedule(dynamic, 64)` to better balance workload across threads for varying node degrees.

### Part 2: Bottom-Up BFS
**Synchronization**: Used `#pragma omp atomic` to safely update the new frontier count when nodes join.

**Overhead Mitigation**: Used dynamic scheduling with a larger chunk size (`1024`) since this phase can be more compute-heavy but has fewer additions to the frontier.

---

### Part 3: Hybrid BFS
**Switching Strategy**: The hybrid method checks:
- **Frontier density**: ratio of frontier size to total nodes.
- **Average out-degree** of frontier nodes.

If the frontier is sparse or sparsely connected, it uses **top-down**. If dense, it switches to **bottom-up** to reduce the cost of checking many neighbors.

This strategy tries to reduce the inefficiencies of each method depending on the traversal phase.

---

## Performance Tuning and Measurements

To optimize performance:
- We added timers around each BFS iteration (using `CycleTimer`).
- Observed that top-down BFS struggles as the frontier becomes large due to excessive synchronization.
- Bottom-up performs better in later levels with a denser frontier.
- Hybrid combines both benefits and improves overall traversal time.

---

## Why Perfect Speedup is Not Achieved

- **Workload Imbalance**: Nodes have widely varying degrees; some threads may process far more neighbors.
- **Synchronization Overhead**: Atomic operations are still expensive at scale.
- **Memory Bandwidth & Cache Contention**: High degree of random memory accesses across threads impacts performance.
- **Switching Logic**: Hybrid decisions aren't perfect and may lead to suboptimal choices occasionally.

Despite optimizations, these factors prevent perfect linear speedup.
