#include "messaging.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"
#include "main.h"
#include "util.h"

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


  if (rank == ROOT_PROCESS) {
    // Send matrix to slaves
    int i, start_ix, send_length;

    /* Send each process its rows, including the rows above and below */
    for (i = 1; i < nprocesses; ++i) {
      // first element for proc i to relax
      start_ix = p_data[i].start_ix;
      send_length = p_data[i].nelements;

      // Add preceding row if necessasry
      // the first process will always take the top line
      if (i != 0) {
        start_ix -= dim;
        send_length += dim;
      }
      // Add following row if necessary
      // The last process will always take the bottom line
      if (i != nprocesses - 1) {
        send_length += dim;
      }
      if (v)
        printf("Sending elems [%d-%d) to process %d to work on [%d-%d)...\n",
               start_ix,
               start_ix + send_length,
               i,
               p_data[i].start_ix, p_data[i].end_ix );
      MPI_Send( &data[start_ix],
                send_length,
                MPI_FLOAT,
                i,
                send_data_tag,
                MPI_COMM_WORLD                  );

      if (v) printf("Successfuly sent work to process %d!\n", i);
    }
  } else {
    // dim is the index of the first relevant element
    printf("SLAVE %d: Returning %d elements starting from index: %d\n",
           rank, p_data[rank].nelements, dim);
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
    int i, recv_ix = p_data[ROOT_PROCESS].nelements, recv_length;

    /* Read the array back in chunks */
    for (i = 1; i < nprocesses; ++i) {
      recv_length = p_data[i].nelements;
      if (v)
        printf("MASTER: Receiving %d elems starting from %d from process %d\n",
               recv_length, recv_ix, i);

      MPI_Recv( &data[recv_ix], recv_length,          MPI_FLOAT,
                i,   return_data_tag, MPI_COMM_WORLD,
                &status);

      recv_ix += recv_length;
    }
  } else {
    int recv_length = p_data[rank].nelements;
    if (rank == 0) {
      recv_length += dim;
    }
    if (rank == nprocesses - 1) {
      recv_length += dim;
    }

    if (v) printf("Slave (%d) waiting for matrix\n...", rank);
    MPI_Recv( data,         recv_length,       MPI_FLOAT,
              ROOT_PROCESS, send_data_tag, MPI_COMM_WORLD,
              &status );
    if (v) printf("Slave (%d) received matrix successfully!\n", rank);
    if (v) print_matrix(data, recv_length, dim);
  }
  printf("Matrix received.\n");
}
