# $ make
# $ ./boids

CC = g++
CFLAGS = -c
STD = -std=c++11
DEBUG = -g
SFML = -lsfml-graphics -lsfml-window -lsfml-system

all: graphs

graphs: Tile.o Game.o Astar.o BFS.o Dijkstra.o main.o
	$(CC) $(STD) $(DEBUG) Tile.o Game.o Astar.o BFS.o Dijkstra.o main.o -o graphs $(SFML)

Tile.o : Tile.cpp
	$(CC) $(STD) $(CFLAGS) $(DEBUG) Tile.cpp

Game.o : Game.cpp
	$(CC) $(STD) $(CFLAGS) $(DEBUG) Game.cpp

Astar.o : Astar/Astar.cpp
	$(CC) $(STD) $(CFLAGS) $(DEBUG) Astar/Astar.cpp

BFS.o : BFS/BFS.cpp
	$(CC) $(STD) $(CFLAGS) $(DEBUG) BFS/BFS.cpp

Dijkstra.o : Dijkstra/Dijkstra.cpp
	$(CC) $(STD) $(CFLAGS) $(DEBUG) Dijkstra/Dijkstra.cpp

.PHONY: clean
clean:
	-rm *.o graphs
