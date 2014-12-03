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
int dim = 0,
    length = 0,
    nprocesses,
    min_elements_per_process,
    min_rows_per_process,
    iter_counter,
    rank,
    start_ix,
    end_ix,
    start_row,
    end_row,
    rc; // MPI response code

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

  if (V) printf("\nAssigning Work and Starting Parallel Section\n");
  if (V) printf("============================================\n\n");


  /* Send matrix to processes */
  for (i = 1; i < nprocesses; ++i) {
    send_matrix(dim, arr, i);

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
}

/*
 * Sets the variables which tell the processes which part of the array to work
 * on
 */
void calculate_work_area(int rank) {
  start_ix = rank * min_elements_per_process;
  end_ix = start_ix + min_elements_per_process;

  start_row = rank * min_rows_per_process;
  end_row = start_row + min_rows_per_process;

  // Give leftovers to last process
  if (rank == nprocesses - 1) {
    end_ix += length % nprocesses;
    end_row += dim % nprocesses;
  }

  if (v)
    printf(
      "This processor (%d) will work on elems [%d, %d] or rows [%d, %d]\n",
      rank, start_ix, end_ix, start_row, end_row );
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
void run_master(void);
void run_slave(void);
void establish_gbls(void);

int main (int argc, char *argv[]) {


  init(argc, argv);
  if (rank == ROOT_PROCESS) {
    run_master();
  } else {
    run_slave();
  }

  MPI_Finalize();
  return 0;
}

/*
 * Initialisation relevant to both master and slaves
 */
void init(int argc, char *argv[]) {
  rc = MPI_Init(&argc, &argv); // Starts MPI

  /* Check successful startup */
  if (rc != MPI_SUCCESS) {
    printf("Error starting MPI program. Terminating.\n");
    MPI_Abort(MPI_COMM_WORLD, rc);
  }

  parse_args(argc, argv);
  establish_gbls();
}

/*
 * Handles the delegation of work and then the subsequent reduction of both
 * the return array rows, as well as precision value to check for completion
 */
void run_master(void) {
  printf("\nSTARTING MASTER EXECUTION\n");
  printf("================\n\n");

  /* Branch for master and slave execution */
  printf("This is the master! (process %d of %d)\n", rank, nprocesses);
  assign_work();
}

void run_slave(void) {
  /* Slave process execution */
  printf("This is a slave! (process %d of %d)\n", rank, nprocesses);
  receive_work();
}

/* Calculate useful values at startup */
void establish_gbls(void) {
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process id
  MPI_Comm_size(MPI_COMM_WORLD, &nprocesses); // Get number of processes

  min_elements_per_process = length / nprocesses;
  min_rows_per_process = dim / nprocesses;

  calculate_work_area(rank);

  if (v) printf("Min elements per process: %d\n\n", min_elements_per_process);
  if (v) printf("Min rows per process: %d\n\n", min_rows_per_process);
}

