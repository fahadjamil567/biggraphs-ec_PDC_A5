#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <omp.h>
#include <atomic>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

void vertex_set_clear(vertex_set *list)
{
    list->count = 0;
}

void vertex_set_init(vertex_set *list, int count)
{
    list->max_vertices = count;
    list->vertices = (int *)malloc(sizeof(int) * list->max_vertices);
    vertex_set_clear(list);
}

void top_down_step(
    Graph g,
    vertex_set *frontier,
    vertex_set *new_frontier,
    int *distances)
{
#pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < frontier->count; i++)
    {
        int node = frontier->vertices[i];
        int start_edge = g->outgoing_starts[node];
        int end_edge = (node == g->num_nodes - 1)
                           ? g->num_edges
                           : g->outgoing_starts[node + 1];

        for (int neighbor = start_edge; neighbor < end_edge; neighbor++)
        {
            int outgoing = g->outgoing_edges[neighbor];

            if (__sync_bool_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, distances[node] + 1))
            {
                int index;
#pragma omp atomic capture
                index = new_frontier->count++;

                new_frontier->vertices[index] = outgoing;
            }
        }
    }
}

void bfs_top_down(Graph graph, solution *sol)
{
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);

        top_down_step(graph, frontier, new_frontier, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        vertex_set *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }
}

// Bottom-up BFS step
void bottom_up_step(
    Graph g,
    vertex_set *frontier,
    vertex_set *new_frontier,
    int *distances,
    int current_level)
{
#pragma omp parallel for schedule(dynamic, 1024)
    for (int node = 0; node < g->num_nodes; node++)
    {
        if (distances[node] != NOT_VISITED_MARKER)
            continue;

        int start_edge = g->incoming_starts[node];
        int end_edge = (node == g->num_nodes - 1)
                           ? g->num_edges
                           : g->incoming_starts[node + 1];

        for (int edge = start_edge; edge < end_edge; edge++)
        {
            int parent = g->incoming_edges[edge];

            if (distances[parent] == current_level)
            {
                distances[node] = current_level + 1;
                int index;
#pragma omp atomic capture
                {
                    index = new_frontier->count++;
                }
                new_frontier->vertices[index] = node;

                break; // Found a parent in frontier, no need to check other parents
            }
        }
    }
}

void bfs_bottom_up(Graph graph, solution *sol)
{
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    for (int i = 0; i < graph->num_nodes; i++)
    {
        sol->distances[i] = NOT_VISITED_MARKER;
    }

    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;
    int level = 0;

    while (frontier->count > 0)
    {
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
        vertex_set *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }
}

void hybrid_step(Graph g, vertex_set *frontier, vertex_set *new_frontier, int *distances, int level)
{
    if (frontier->count < g->num_nodes / 2)
    {
        // uses top down if frontier small
        top_down_step(g, frontier, new_frontier, distances);
    }
    else
    {
        // uses bottom up in case frontier large
        bottom_up_step(g, frontier, new_frontier, distances, level);
    }
}

void bfs_hybrid(Graph graph, solution *sol)
{
    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *new_frontier = &list2;

    for (int i = 0; i < graph->num_nodes; i++)
    {
        sol->distances[i] = NOT_VISITED_MARKER;
    }

    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;
    int level = 0;

    while (frontier->count > 0)
    {
#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(new_frontier);
        hybrid_step(graph, frontier, new_frontier, sol->distances, level);
        level++;

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swapping frontiers
        vertex_set *tmp = frontier;
        frontier = new_frontier;
        new_frontier = tmp;
    }
}
