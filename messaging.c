#include "messaging.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"
#include "main.h"

struct process_data;

extern bool v, V;
extern int length, dim, nprocesses;
extern process_data *p_data;

/*
 * If rank is the ROOT_PROCESS,
 *
 * Sends the entire matrix (with length defined in the variable "length")
 * Sends the row above and below for all rows other than the top and bottom
 *
 * else
 *
 * Sends $nelements elements after the first $dim elements
 * This removes the extranous top and bottom rows
 */
void send_matrix(float *data, int rank) {
  printf("Sending array of length: %d and dim: %d to process %d...\n",
        length, dim, rank);

  if (rank == ROOT_PROCESS) {
    // Send matrix to slaves
    int i, start_ix, send_length;

    /* Send each process its rows, including the rows above and below */
    for (i = 0; i < nprocesses; ++i) {
      start_ix = i * dim; // first element for proc i to relax
      send_length = p_data[i].nelements;

      // Add preceding row if necessasry
      if (i != 0) {
        start_ix -= dim;
        send_length += dim;
      }
      // Add following row if necessary
      if (i != nprocesses) {
        send_length += dim;
      }
      MPI_Send( &data[start_ix],
                send_length,
                MPI_FLOAT,
                rank,
                send_data_tag,
                MPI_COMM_WORLD                  );
    }
  } else {
    // dim is the index of the first relevant element
    MPI_Send( &data[dim],
              p_data[rank].nelements,
              MPI_FLOAT,
              ROOT_PROCESS,
              return_data_tag,
              MPI_COMM_WORLD           );
  }
}

/*
 * If rank is the ROOT_PROCESS,
 *
 * Receives responses from  the other nprocesses - 1 processes with
 * return_data_tag
 *
 * else
 *
 * Reads whole matrix with send_data_tag
 */
void receive_matrix(float *data, int rank) {
  MPI_Status status;

  if (rank == ROOT_PROCESS) {
    int i, recv_ix = 0, recv_length;

    /* Read the array back in chunks */
    for (i = 1; i < nprocesses; ++i) {
      recv_length = p_data[i].nelements;
      MPI_Recv( &data[recv_ix], recv_length,          MPI_FLOAT,
                i,   return_data_tag, MPI_COMM_WORLD,
                &status);

      recv_ix += recv_length;
    }
  } else {
    MPI_Recv( data,         length,       MPI_FLOAT,
              ROOT_PROCESS, send_data_tag, MPI_COMM_WORLD,
              &status );
  }
  printf("Matrix received.\n");
}
