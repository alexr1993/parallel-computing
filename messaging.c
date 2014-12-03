#include "messaging.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

#define send_data_tag 42

extern bool v, V;

void send_matrix(int dim, float *data, int rank) {
    // Send array length
    MPI_Send( &dim, 1,             MPI_INT,
              rank,    send_data_tag, MPI_COMM_WORLD );

    printf("Sending array of length: %d and dim: %d to process %d...\n",
          dim * dim, dim, rank);

    // Send matrix (array)
    MPI_Send( data, dim * dim, MPI_FLOAT, rank, send_data_tag, MPI_COMM_WORLD );
}

float *receive_matrix(int *length, int *dim) {
  MPI_Status status;
  // Receive array size
  MPI_Recv( dim,         1,              MPI_INT,
            ROOT_PROCESS, send_data_tag,  MPI_COMM_WORLD,
            &status );

  *length = *dim * *dim; // TODO sort out this mess

  printf("Received int length: %d\n", *length);

  float *data = malloc(*length * sizeof(float));

  MPI_Recv( data,         *length,       MPI_FLOAT,
            ROOT_PROCESS, send_data_tag, MPI_COMM_WORLD,
            &status );
  printf("Matrix received too\n");
  return data;
}

void send_process_data(int start_ix, int end_ix, int rank) {
  int indexes[2] = { start_ix, end_ix };
  if (V) printf("Sending [%d, %d] to process %d.\n", start_ix, end_ix, rank);
  MPI_Send(indexes, 2, MPI_INT, rank, send_data_tag, MPI_COMM_WORLD);
}

void receive_process_data(int *start_ix_ptr, int *end_ix_ptr) {
  int indexes[2] = { 0, 0 };
  MPI_Status status;
  MPI_Recv(indexes, 2, MPI_INT, ROOT_PROCESS, send_data_tag, MPI_COMM_WORLD,
           &status);

  *start_ix_ptr = indexes[0];
  *end_ix_ptr = indexes[1];
}
