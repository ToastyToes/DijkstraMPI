#ifndef _GRAPH_H
#define _GRAPH_H

#define ROOT 0
#define TRUE 1
#define FALSE 0
#define LENGTH 0
#define SOURCE 1
#define UNDEF -1
#define SIZE_ONE 1
#define SIZE_TWO 2

// +ve & -ve infinity:
#define INF 1000
#define NEG_INF -1000

// Tags used for passing messages:
#define LENGTH_TAG 1
#define ACKNOWLEDGE_TAG 2
#define OVER_TAG 3
#define NEG_CYCLE_TAG 4

// globals:
int *row;       // A row of the adjacency matrix, that a process receives
int *matrix;    // Adjacency matrix
int my_rank;    // Rank of the current process running
int vertices;   // Total number of vertices in the graph

int *runRoot(); // Runs the root process's computation
int *runRest(); // Runs the rest of the processes' computation
void dijkstra();    // Starts all processes
void freeMemory();  // Free's the memory occupied by the adjacency matrix
void printGraph();  // Prints the adjacency matrix
void create_graph();    // Creates the adjacency matrix by reading the graph.txt file
int *getAckMessage();   // Returns the acknowledgment message received by a process
int *getLengthMessage();    // Returns the length message received by a process
int messageReceived(int tag);   // Returns true if a message with the given tag has been received
void sendAckMessage(int dest);  // Sends acknowledgment message to a destination/process
void sendOverMessage(int tag);  // Send an over message with a given tag
int getMatrixIndex(int row, int col);   // Calculates and returns the proper index for the adjacency matrix
void sendLengthMessage(int dest, int weight);   // Sends length messages with a given weight/path length to a destination/process
void printResult(int *arrayLength, int *arrayPred); // Prints the final results of Dijkstra

#endif