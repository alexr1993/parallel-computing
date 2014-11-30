#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mpi.h"

#include "util.h"
#include "relaxation.h"
#include "messaging.h"

#define send_data_tag 2001
#define return_data_tag 2002
#define ROOT_PROCESS 0

bool v, V; //verbose
int dim = 0, length = 0, nprocesses, min_elements_per_process, iter_counter, rank, rc;
float precision;
float *arr, *new_values_arr, *precision_arr, *incoming_arr;

struct process_data;
struct process_data p_data;
struct process_data *process_data_arr;

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

  /* Send matrix to processes */
  /* Send processes their work spec (start and end indexes) */
  int start_ix, end_ix;

  for (i = 1; i < nprocesses; ++i) {
    send_matrix(dim, arr, i);

    start_ix = i * nprocesses;
    end_ix = start_ix + min_elements_per_process;

    // Give leftovers to last process
    if (i == nprocesses - 1) {
      end_ix += length % nprocesses;
    }
    send_process_data(start_ix, end_ix, i);
  }
}

/*
 * (Slave)
 * MPI_Recv's an array length and an array
 * TODO Separate into slave_init and receive_work
 */
void receive_work(void) {
  printf("Slave process receiving work...\n");

  arr = receive_matrix(&length, &dim);

  /* Print some received data to check */
  if (rank == 1) print_matrix(arr, length, dim);

  int start_ix, end_ix;
  receive_process_data(&start_ix, &end_ix);
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
  free(filename);
}
void init(int argc, char *argv[]);
void run_master(int argc, char *argv[]);
void run_slave(void);

int main (int argc, char *argv[]) {

  init(argc, argv);
  if (rank == ROOT_PROCESS) {
    run_master(argc, argv);
  } else {
    run_slave();
  }

  MPI_Finalize();
  return 0;
}

void init(int argc, char *argv[]) {
  rc = MPI_Init(&argc, &argv); // Starts MPI

  /* Check successful startup */
  if (rc != MPI_SUCCESS) {
    printf("Error starting MPI program. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process id
  MPI_Comm_size(MPI_COMM_WORLD, &nprocesses); // Get number of processes
}

void run_master(int argc, char *argv[]) {
  printf("\nSTARTING MASTER EXECUTION\n");
  printf("================\n\n");

  /* Branch for master and slave execution */
  printf("This is the master! (process %d of %d)\n", rank, nprocesses);
  parse_args(argc, argv);
  assign_work();
}

void run_slave(void) {
  /* Slave process execution */
  printf("This is a slave! (process %d of %d)\n", rank, nprocesses);
  receive_work();
}
