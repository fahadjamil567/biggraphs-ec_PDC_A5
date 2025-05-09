#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <omp.h>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

void vertex_set_clear(vertex_set* list) {
    list->count = 0;
}

void vertex_set_init(vertex_set* list, int count) {
    list->max_vertices = count;
    list->vertices = (int*)malloc(sizeof(int) * list->max_vertices);
    vertex_set_clear(list);
}

// Top-down BFS step
void top_down_step(
    Graph g,
    vertex_set* frontier,
    vertex_set* new_frontier,
    int* distances)
{
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < frontier->count; i++) {
        int node = frontier->vertices[i];
        int start_edge = g->outgoing_starts[node];
        int end_edge = (node == g->num_nodes - 1)
                           ? g->num_edges
                           : g->outgoing_starts[node + 1];

        for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
            int outgoing = g->outgoing_edges[neighbor];

            // Atomically check and set the distance
            if (__sync_bool_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, distances[node] + 1)) {
                // Atomic fetch and add to get unique index
                int index;
                #pragma omp atomic capture
                index = new_frontier->count++;

                new_frontier->vertices[index] = outgoing;
            }
        }
    }
}

// Top-Down BFS implementation
void bfs_top_down(Graph graph, solution* sol) {
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set* frontier = &list1;
    vertex_set* new_frontier = &list2;

    // initialize all nodes to NOT_VISITED
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0) {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);

        top_down_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap pointers
        vertex_set* tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }
}

// Bottom-up BFS step
void bottom_up_step(
    Graph g,
    vertex_set* frontier,
    vertex_set* new_frontier,
    int* distances,
    int current_level)
{
    #pragma omp parallel for schedule(dynamic, 1024)
    for (int node = 0; node < g->num_nodes; node++) {
        // Already visited? Skip
        if (distances[node] != NOT_VISITED_MARKER) continue;

        int start_edge = g->incoming_starts[node];
        int end_edge = (node == g->num_nodes - 1)
                           ? g->num_edges
                           : g->incoming_starts[node + 1];

        for (int edge = start_edge; edge < end_edge; edge++) {
            int incoming = g->incoming_edges[edge];

            if (distances[incoming] == current_level) {
                distances[node] = current_level + 1;

                // Add to new frontier
                int index;
                #pragma omp critical
                {
                    index = new_frontier->count++;
                    new_frontier->vertices[index] = node;
                }

                break; // Stop after finding one frontier parent
            }
        }
    }
}

// Bottom-Up BFS implementation
void bfs_bottom_up(Graph graph, solution* sol) {
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set* frontier = &list1;
    vertex_set* new_frontier = &list2;

    // Init all distances
    for (int i = 0; i < graph->num_nodes; i++) {
        sol->distances[i] = NOT_VISITED_MARKER;
    }

    // Start from root
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;
    int level = 0;

    while (frontier->count > 0) {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);
        bottom_up_step(graph, frontier, new_frontier, sol->distances, level);
        level++;

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap frontiers
        vertex_set* tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }
}

// Hybrid BFS step (select between top-down and bottom-up)
void hybrid_step(Graph g, vertex_set* frontier, vertex_set* new_frontier, int* distances, int level) {
    if (frontier->count < g->num_nodes / 2) {
        // Use Top-Down if the frontier is small
        top_down_step(g, frontier, new_frontier, distances);
    } else {
        // Use Bottom-Up if the frontier is large
        bottom_up_step(g, frontier, new_frontier, distances, level);
    }
}

// Hybrid BFS implementation
void bfs_hybrid(Graph graph, solution* sol) {
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set* frontier = &list1;
    vertex_set* new_frontier = &list2;

    // Initialize all distances
    for (int i = 0; i < graph->num_nodes; i++) {
        sol->distances[i] = NOT_VISITED_MARKER;
    }

    // Start from root
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;
    int level = 0;

    while (frontier->count > 0) {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);

        // Use the hybrid step to dynamically choose top-down or bottom-up
        hybrid_step(graph, frontier, new_frontier, sol->distances, level);

        level++;

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // Swap frontiers
        vertex_set* tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }
}


