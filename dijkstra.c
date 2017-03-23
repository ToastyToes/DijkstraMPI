#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "dijkstra.h"
#include "mpi.h"

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); //grab this process's rank

    if(my_rank == ROOT)
        create_graph();

    MPI_Bcast(&vertices, SIZE_ONE, MPI_INT, ROOT, MPI_COMM_WORLD);
    row = malloc(sizeof(int) * vertices);
    MPI_Scatter(matrix, vertices, MPI_INT, row, vertices, MPI_INT, ROOT, MPI_COMM_WORLD);

    dijkstra();

    return EXIT_SUCCESS;
}


void dijkstra()
{
    double start, end;
    int *result;
    int lengths[vertices], predecessors[vertices];

    if (my_rank == ROOT) {
        time_t now = time(0); // Get the system time
        printf("Dijkstra running... \nStart time: %s\n", ctime(&now));
        start = MPI_Wtime();
        result = runRoot();
    } else
        result = runRest();

    MPI_Gather(&result[0], 1, MPI_INT, lengths, 1, MPI_INT, 0, MPI_COMM_WORLD); // Gathers the path length from each process
    MPI_Gather(&result[1], 1, MPI_INT, predecessors, 1, MPI_INT, 0, MPI_COMM_WORLD);    // Gathers the predecessor from each process

    if (my_rank == ROOT) {
        end = MPI_Wtime();
        printf("Finished with total time (sec): %f\n", end - start);
        printResult(lengths, predecessors);
    }
    MPI_Finalize();
}


int *runRoot()
{
    int num_ack = 0;    // tracks number of length messages sent out
    int pred = UNDEF;   // predecessor of the p0
    int succ;           // successor of the p0
    int length = 0;     // shortest length of paths from p0 to p0
    int *message;       // stores incoming messages
    int done = FALSE;
    int *result = malloc(sizeof(int) * SIZE_TWO);

    for (succ = 0; succ < vertices; succ++) { // send (W1k, p1) to all successors pk
        if (row[succ] != 0) { // no edge between vertices
            sendLengthMessage(succ, row[succ]);
            num_ack++;
        }
    }

    while (done != TRUE) {
        if (messageReceived(LENGTH_TAG)) {
            message = getLengthMessage();
            if (message[LENGTH] < 0) {
                done = TRUE;
                result[0] = length;
                result[1] = pred;
                sendOverMessage(NEG_CYCLE_TAG);
            } else {
                sendAckMessage(message[SOURCE]);
            }
        }

        if (messageReceived(ACKNOWLEDGE_TAG)) {
            message = getAckMessage();
            num_ack --;
            if (num_ack == 0) {
                done = TRUE;
                result[0] = length;
                result[1] = pred;
                sendOverMessage(OVER_TAG);
            }
        }
    }
    freeMemory();
    return result;
}


int *runRest() // runs process Pj, where j != 1
{
    int length = INF;   // shortest length of paths from P0 to Pj
    int pred = UNDEF;   // predecessor of the Pj
    int num_ack = 0;    // tracks number of length messages sent out
    int succ;           // successor of the Pj
    int *message;       // stores incoming messages
    int done = FALSE;
    int *result = malloc(sizeof(int) * SIZE_TWO);

    while (done != TRUE) {

        // Phase II:
        if (num_ack > 0) {
            if ( messageReceived(NEG_CYCLE_TAG) || messageReceived(OVER_TAG) ) {
                if (length != NEG_INF) {
                    length = NEG_INF;
                    sendOverMessage(NEG_CYCLE_TAG);
                }
                done = TRUE;
                result[0] = length;
                result[1] = pred;
            }
        }

        if (num_ack == 0) {
            if ( messageReceived(NEG_CYCLE_TAG) && length != NEG_INF ) {
                done = TRUE;
                result[0] = length;
                result[1] = pred;
                sendOverMessage(NEG_CYCLE_TAG);
            }

            if ( messageReceived(OVER_TAG) && length != NEG_INF ) {
                done = TRUE;
                result[0] = length;
                result[1] = pred;
                sendOverMessage(OVER_TAG);
            }
        }

        // Phase I:
        if (messageReceived(LENGTH_TAG)) {
            message = getLengthMessage();
            if (message[LENGTH] < length) {
                if (num_ack > 0) {
                    sendAckMessage(pred);
                }
                pred = message[SOURCE];
                length = message[LENGTH];
                for (succ = 0; succ < vertices; succ++) { // send (new_shortest_length, my_rank) to all successors pk
                    if (row[succ] != 0) { // no edge between vertices
                        sendLengthMessage(succ, (row[succ] + length));
                        num_ack++;
                    }
                }
                if (num_ack == 0)
                    sendAckMessage(pred);
            } else {
                sendAckMessage(message[SOURCE]);
            }
        }

        if (messageReceived(ACKNOWLEDGE_TAG)) {
            message = getAckMessage();
            num_ack --;
            if (num_ack == 0)
                sendAckMessage(pred);
        }

    }
    return result;
}


int messageReceived(int tag)
{
    int flag;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag, &status);
    return flag;
}


void sendOverMessage(int tag)
{
    int msg = my_rank;
    MPI_Request request;
    MPI_Status status;
    int succ;
    for (succ = 0; succ < vertices; succ++) {
        if (row[succ] != 0) {
            MPI_Isend(&msg, SIZE_ONE, MPI_INT, succ, tag, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, &status);
        }
    }

}


void sendLengthMessage(int dest, int weight)
{
    int msg[SIZE_TWO] = {weight, my_rank};
    MPI_Request request;
    MPI_Status status;
    MPI_Isend(msg, SIZE_TWO, MPI_INT, dest, LENGTH_TAG, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
}


void sendAckMessage(int dest)
{
    int msg = my_rank;
    MPI_Request request;
    MPI_Status status;
    MPI_Isend(&msg, SIZE_ONE, MPI_INT, dest, ACKNOWLEDGE_TAG, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, &status);
}


int *getLengthMessage()
{
    int *msg = malloc(sizeof(int) * SIZE_TWO);
    MPI_Status status;
    MPI_Recv(msg, SIZE_TWO, MPI_INT, MPI_ANY_SOURCE, LENGTH_TAG, MPI_COMM_WORLD, &status);
    return msg;
}


int *getAckMessage()
{
    int *msg = malloc(sizeof(int));
    MPI_Status status;
    MPI_Recv(msg, SIZE_ONE, MPI_INT, MPI_ANY_SOURCE, ACKNOWLEDGE_TAG, MPI_COMM_WORLD, &status);
    return msg;
}


void create_graph()
{
    int i, row, col, weight, index;

    FILE *file = fopen("graph.txt", "r");
    fscanf(file, "%d", &vertices);
    matrix = malloc(vertices * vertices * sizeof(int*));

    for(i=0; i<vertices * vertices; i++) {
        matrix[i] = 0;
    }

    while(!feof(file)){
        fscanf(file, "%d %d %d", &row, &col, &weight);
        index = getMatrixIndex(row, col);
        matrix[index] = weight;
    }
    fclose(file);
}


int getMatrixIndex(int row, int col)
{
    int index;
    assert(row >= 0 && col >= 0);
    index = vertices * (row - 1);
    index += col - 1;
    return index;
}


void printResult(int *arrayLength, int *arrayPred)
{
    int i;
    int neg_cycle_found = FALSE;

    for (i=0; i<vertices && !neg_cycle_found; i++) {
        if (arrayLength[i] == NEG_INF)
            neg_cycle_found = TRUE;
    }
    printf("Results:\n\n");
    if (!neg_cycle_found) {
        printf("P:\t\t");
        for (i=0; i<vertices; i++)
            printf("%6d\t", i+1);
        printf("\nLen:\t");
        for (i=0; i<vertices; i++)
            printf("%6d\t", arrayLength[i]);
        printf("\nPred:\t");
        for (i=0; i<vertices; i++)
            printf("%6d\t", arrayPred[i]);
        printf("\n");
    } else {
        printf("A negative cycle was detected.\n");
    }
}


void printGraph()
{
    int i, j, count = 0;
    printf("\tTotal vertices: %d\n", vertices);

    for(i=0; i<(vertices*vertices); i+=vertices) {
        for(j=0; j<vertices; j++) {
            printf("\t%2d", matrix[i+j]);
        }
        count++;
        printf("\n");
    }
}


void freeMemory()
{
    free(matrix);
}
