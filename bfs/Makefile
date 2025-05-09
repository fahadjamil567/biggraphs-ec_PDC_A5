CXX = g++
CXXFLAGS = -I../ -std=c++11 -fopenmp -O3 -g

COMMON_OBJS = ../common/graph.cpp ref_bfs.o

all: bfs bfs_grader

bfs: main.cpp bfs.cpp $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o bfs main.cpp bfs.cpp $(COMMON_OBJS)

bfs_grader: grade.cpp bfs.cpp $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o bfs_grader grade.cpp bfs.cpp $(COMMON_OBJS)

clean:
	rm -rf bfs bfs_grader *~ *.*~

