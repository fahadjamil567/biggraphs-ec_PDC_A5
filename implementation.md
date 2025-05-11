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

When we first tackled this assignment, we knew that parallelizing BFS would be challenging because of its data-dependent nature. But anyways here's how we approached each part of the implementation and what we learned along the way.


### Top-Down BFS Implementation

For our top-down approach, we started with a simple idea: let's divide the frontier vertices among all available threads. We used OpenMP's parallel for construct with dynamic scheduling:

**Synchronization**: Used `__sync_bool_compare_and_swap()` to atomically update the `distances` array and an `#pragma omp atomic` to safely increment the new frontier count.

**Overhead Mitigation**: Limited synchronization to only necessary updates. Also used `schedule(dynamic, 64)` to better balance workload across threads for varying node degrees.


```cpp
#pragma omp parallel for schedule(dynamic, 64)
for (int i = 0; i < frontier->count; i++) {
    // Process each node in the frontier
}
```

#### Where synchronization happens:
1. **Marking nodes as visited** using atomic compare-and-swap:
```cpp
__sync_bool_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, distances[node] + 1)
```

2. **Adding to the new frontier** using atomic capture:
```cpp
#pragma omp atomic capture
index = new_frontier->count++;
```

We found chunk size 64 to be optimal—balancing scheduling overhead and thread utilization.

---

### Bottom-Up BFS Implementation

Instead of frontier nodes exploring neighbors, we flipped it: **each unvisited node checks if any of its incoming neighbors are in the frontier**.

**Synchronization**: Used `#pragma omp atomic` to safely update the new frontier count when nodes join.

**Overhead Mitigation**: Used dynamic scheduling with a larger chunk size (`1024`) since this phase can be more compute-heavy but has fewer additions to the frontier.


```cpp
#pragma omp parallel for schedule(dynamic, 1024)
for (int node = 0; node < g->num_nodes; node++) {
    // Skip already visited nodes
    if (distances[node] != NOT_VISITED_MARKER)
        continue;

    // Look for parents in the frontier
    // ...
}
```

#### Key optimization:
- **Early termination** with `break` as soon as a parent is found in the frontier.
- Larger chunk size (1024) minimized synchronization overhead, as fewer nodes joined the frontier per iteration.

---

### Hybrid BFS Implementation – The Best of Both Worlds

This was the most interesting part! We dynamically switched between top-down and bottom-up based on:

**Switching Strategy**: The hybrid method checks:
1. **Frontier density**: Frontier size relative to total nodes.
2. **Average out-degree** of frontier nodes.

```cpp
double frontier_density = (double)frontier->count / g->num_nodes;

if (frontier_density < 0.1 || (frontier_density < 0.3 && avg_out_degree > 10)) {
    top_down_step(g, frontier, new_frontier, distances);
} else {
    bottom_up_step(g, frontier, new_frontier, distances, level);
}
```

If the frontier is sparse or sparsely connected, it uses **top-down**. If dense, it switches to **bottom-up** to reduce the cost of checking many neighbors.

This strategy tries to reduce the inefficiencies of each method depending on the traversal phase.

---


## Performance Observations

We used `CycleTimer` to measure BFS iterations and guide our optimizations. We observed:

- **Top-down** excels in sparse graphs and early levels.
- **Bottom-up** outperforms in later levels with dense frontiers.
- **Hybrid** bridges both—adapting to graph structure dynamically.
---
## Performance Tuning and Measurements

To optimize performance:
- We added timers around each BFS iteration (using `CycleTimer`).
- Observed that top-down BFS struggles as the frontier becomes large due to excessive synchronization.
- Bottom-up performs better in later levels with a denser frontier.
- Hybrid combines both benefits and improves overall traversal time.

---

## Why Perfect Speedup is Not Achieved

Despite optimizations, perfect linear speedup is difficult due to:

1. **Memory Bottlenecks**: BFS is memory-bound; multiple threads competing for memory slow things down.
2. **Workload Imbalance**: Power-law graphs have high-degree hubs, skewing work distribution.
3. **Atomic Contention**: Synchronization on shared variables introduces delay.
4. **Frontier Overhead**: Managing dynamic frontiers and hybrid switching adds compute not directly tied to traversal.
5. **Synchronization Overhead**: Atomic operations are still expensive at scale.
6. **Memory Bandwidth & Cache Contention**: High degree of random memory accesses across threads impacts performance.
7. **Switching Logic**: Hybrid decisions aren't perfect and may lead to suboptimal choices occasionally.

---

## What We Learned

We found that synchronization, though essential for correctness, significantly limits scalability. More than low-level optimizations, the overall algorithm design—such as switching between top-down and bottom-up approaches—had a much greater impact on performance. Adaptive strategies proved especially effective, as different phases of BFS benefit from different traversal techniques. Ultimately, we observed that memory access patterns dominate performance in irregular, data-driven applications like BFS.


- **Synchronization is necessary but limits scalability.**
- **Algorithm design (e.g., switching strategies) has a bigger impact than fine-tuning.**
- **Memory patterns dominate performance** 