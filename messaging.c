#include "messaging.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

extern bool v, V;
extern int length, dim;
extern float *arr;


/*
 * Sends the entire matrix (with length defined in the variable "length")
 */
void send_matrix(float *data, int rank) {
    printf("Sending array of length: %d and dim: %d to process %d...\n",
          length, dim, rank);

    // Send matrix (array)
    MPI_Send( data, dim * dim, MPI_FLOAT, rank, send_data_tag, MPI_COMM_WORLD );
}

/*
 * Stores the incoming array in the global arr variable.
 *
 * Note that arr is the size of the matrix, but in this context it is likely
 * that it will only be filled with a few rows of data
 */
float *receive_matrix(float *data) {
  MPI_Status status;

  MPI_Recv( arr,         length,       MPI_FLOAT,
            ROOT_PROCESS, send_data_tag, MPI_COMM_WORLD,
            &status );
  printf("Matrix received too\n");
  return data;
}
