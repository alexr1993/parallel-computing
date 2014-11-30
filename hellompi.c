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
#define ROOT_PROCESS 0

bool v, V; //verbose
int dim, length, nprocesses, min_elements_per_process, iter_counter, rank, rc;
float precision;
float *arr, *new_values_arr, *precision_arr, *incoming_arr;

struct process_data;

/*
 * Sends processes their array indices
 */
void assign_work(void) {
  /* (For now) send the whole array to each processes */
  int i;
  min_elements_per_process = length / nprocesses;

  if (V) printf("\nAssigning Work and Starting Parallel Section\n");
  if (V) printf("============================================\n\n");
  if (v) printf("Min elements per process: %d\n\n",
                min_elements_per_process);

  for (i = 1; i < nprocesses; ++i) {
    printf("Sending %d to process %d\n", dim, i);
    // Send array length
    MPI_Send( &dim, 1,             MPI_INT,
              i,       send_data_tag, MPI_COMM_WORLD );

    printf("Sending array of length: %d and dim: %d...\n", length, dim);
    print_matrix(arr, length, dim);
    // Send matrix (array)
    MPI_Send( &arr[0], length, MPI_FLOAT, i, send_data_tag, MPI_COMM_WORLD );
  }
}

/*
 * MPI_Recv's an array length and an array
 * TODO Separate into slave_init and receive_work
 */
void receive_work(void) {
  MPI_Status status;

  printf("Slave process receiving work...\n");

  // Receive array size
  MPI_Recv( &dim,         1,              MPI_INT,
            ROOT_PROCESS, send_data_tag,  MPI_COMM_WORLD,
            &status );

  length = dim * dim;

  printf("This thread will have %d rows to receive!\n",
         length);

  incoming_arr = malloc(length * sizeof(float));
  MPI_Recv( &incoming_arr, length, MPI_FLOAT,
            ROOT_PROCESS, send_data_tag, MPI_COMM_WORLD,
            &status );
  printf("Matrix received too\n");

  /* Print to check */
  print_matrix(incoming_arr, length, dim);
}

void parse_args(int argc, char *argv[]) {

  /* Set Default Args */
  char *filename = NULL;
  dim = 50;
  precision  = 0.1;
  v = false;
  V = false;

  /* Parse Args */
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
  /* Initialise Matrix */
  if (filename) {
    arr = read_array(filename, &length, &dim);
  } else {
    printf("Generating matrix as no filename given.\n\n");
    length = dim * dim;
    arr = create_plain_matrix(length, dim);
  }
}

int main (int argc, char *argv[]) {

  rc = MPI_Init(&argc, &argv); // Starts MPI

  /* Check successful startup */
  if (rc != MPI_SUCCESS) {
    printf("Error starting MPI program. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process id
  MPI_Comm_size(MPI_COMM_WORLD, &nprocesses); // Get number of processes

  if (rank == ROOT_PROCESS) {

    printf("\nSTARTING PROGRAM\n");
    printf("================\n\n");

    /* Branch for master and slave execution */
    printf("This is the master! (process %d of %d)\n", rank, nprocesses);
    parse_args(argc, argv);
    assign_work();
  } else {
    /* Slave process execution */
    printf("This is a slave! (process %d of %d)\n", rank, nprocesses);
    receive_work();
  }

  MPI_Finalize();
  return 0;
}
