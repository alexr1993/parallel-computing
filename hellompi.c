#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mpi.h"

#include "util.h"
#include "relaxation.h"

#define send_data_tag 2001
#define return_data_tag 2002

bool v, V; //verbose
int dim, length, nprocesses, min_elements_per_process, iter_counter;
float precision;
float *arr, *new_values_arr, *precision_arr;

struct process_data;

/*
 * Sends processes their array indices
 */
void assign_work(void) {
  /* (For now) send the whole array to each processes */
  int i, nrows_to_send = dim;
  for (i = 1; i < nprocesses; ++i) {
    printf("Sending %d to process %d\n", nrows_to_send, i);
    MPI_Send( &nrows_to_send, 1,             MPI_INT,
              i,              send_data_tag, MPI_COMM_WORLD );
  }
}

/* Creates an array which is all 0s apart from the edges which are 1s */
void init_plain_matrix() {
  arr = malloc(length * sizeof(float));

  int i;
  for (i = 0; i < length; ++i) {
    // Set the edges to 1 and everything else to 0
    if (is_edge_index(i, dim)) {
      arr[i] = 1;
    } else {
      arr[i] = 0;
    }
  }
  if (v) printf("Initiated plain matrix:\n");
  if (v) printf("Matrix length: %d, dimension: %d\n\n", length, dim);
  if (v) print_matrix(arr, length, dim);
}

void parse_args(int argc, char *argv[]) {

  // Default cmd line args
  char *filename = NULL;
  dim = 50;
  precision  = 0.1;
  v = false;
  V = false;

  // Parse args
  int c = 0;
  while ( (c = getopt(argc, argv, "d::f::p::n::v::V::")) != -1 ) {
    switch(c) {
      // dimensions
      case 'd':
        dim = atoi(optarg);
        break;
      // filename
      case 'f':
        filename = malloc(strlen(optarg) * sizeof(char));
        strncpy(filename, optarg, strlen(optarg));
        break;
      // precision
      case 'p':
        precision = atof(optarg);
        break;
      // verbose
      case 'v':
        v = true;
        break;
      // very verbose
      case 'V':
        V = true;
        v = true;
        break;
      default:
        printf("Invalid arg: %s\n", optarg);
        break;
    }
  }

  if (filename) {
    arr = read_array(filename, &length, &dim);
  } else {
    printf("Generating matrix as no filename given.\n\n");
    length = dim * dim;
    init_plain_matrix();
  }
}

int main (int argc, char *argv[]) {

  int rank, rc;

  rc = MPI_Init(&argc, &argv); // Starts MPI

  /* Check successful startup */
  if (rc != MPI_SUCCESS) {
    printf("Error starting MPI program. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process id
  MPI_Comm_size(MPI_COMM_WORLD, &nprocesses); // Get number of processes

  int root_process = 0;
  if (rank == root_process) {

    /* Branch for master and slave execution */
    printf("This is the master! (process %d of %d)\n", rank, nprocesses);
    parse_args(argc, argv);
    print_matrix(arr, length, dim);
    min_elements_per_process = length / nprocesses;

    if (V) printf("\nAssigning Work and Starting Parallel Section\n");
    if (V) printf("============================================\n\n");
    if (v) printf("Min elements per process: %d\n\n",
                  min_elements_per_process);

    assign_work();
  } else {
    /* Slave process execution */
    printf("This is a slave! (process %d of %d)\n", rank, nprocesses);
    MPI_Status status;
    int nrows_to_receive;

    rc = MPI_Recv( &nrows_to_receive, 1, MPI_INT,
                   root_process,      send_data_tag, MPI_COMM_WORLD,
                   &status );

    printf("This thread will have %d rows to receive!\n",
           nrows_to_receive);
  }

  MPI_Finalize();
  return 0;
}
