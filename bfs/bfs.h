#ifndef __BFS_H__
#define __BFS_H__

// #define DEBUG

#include "common/graph.h"

struct solution
{
  int *distances;
};

struct vertex_set
{
  int count;
  int max_vertices;
  int *vertices;
};

void bfs_top_down(Graph graph, solution *sol);
void bfs_bottom_up(Graph graph, solution *sol);
void bfs_hybrid(Graph graph, solution *sol);

#endif
