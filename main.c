#include "main.h"

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

bool v, V; //verbose
int dim = 0,
    length = 0,
    nprocesses,
    min_rows_per_process,
    iter_counter = 0,
    rank,
    start_ix,
    end_ix,
    start_row,
    end_row,
    rc; // MPI response code

float precision, current_precision;
float *arr, *precision_arr, *working_arr, *new_working_arr;

struct process_data *p_data;

/*
 * Sets the variables which tell the processes which part of the array to work
 * on
 */
void calculate_work_area(void) {
  int leftovers = dim % nprocesses;
  int i, bump = 0;
  for (i = 0; i < nprocesses; ++i) {
    p_data[i].start_row = i * min_rows_per_process + bump;
    p_data[i].end_row = p_data[i].start_row + min_rows_per_process;

    // Distribute leftovers
    if (leftovers != 0) {
      ++p_data[i].end_row;
      --leftovers;
      ++bump;
    }
    p_data[i].start_ix = p_data[i].start_row * dim;
    p_data[i].end_ix   = p_data[i].end_row   * dim;
    p_data[i].nrows = p_data[i].end_row - p_data[i].start_row;
    p_data[i].nelements = p_data[i].nrows * dim;
  }
}

void parse_args(int argc, char *argv[]) {

  /* Set Default Args */
  char *filename = NULL;
  dim = 50;
  precision = 0.1;
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
  if (rank == ROOT_PROCESS) {
    /* Initialise Matrix */
    if (filename) {
      arr = read_array(filename, &length, &dim);
    } else {
      printf("Generating matrix as no filename given.\n\n");
      length = dim * dim;
      arr = create_plain_matrix(length, dim);
    }
  }
  free(filename);

  if (rank == ROOT_PROCESS) {
    send_size();
  } else {
    receive_size();
  }
}

void init(int argc, char *argv[]);
void run_master(void);
void run_slave(void);
void establish_gbls(void);
void recover_matrix(void);

int main (int argc, char *argv[]) {

  init(argc, argv);
  if (rank == ROOT_PROCESS) {
    run_master();
  } else {
    if (v)
      printf("SLAVE %d: Starting operation on %d x %d matrix (%d elems).\n",
           rank, dim, dim, length);
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
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process id
  MPI_Comm_size(MPI_COMM_WORLD, &nprocesses); // Get number of processes

  parse_args(argc, argv);
  establish_gbls();
}

int get_rel_end_ix(void) {
  int rel_end_ix = p_data[rank].nelements;
  rel_end_ix += dim * get_nrows_of_padding();
  return rel_end_ix;
}

/* Handles the indexing and relaxing of elements for a process */
int process_relax(float *input_arr, float *output_arr) {
  int rel_start_ix = 0, rel_end_ix = get_rel_end_ix();

  if (V) printf("SLAVE %d: Relaxing elements [%d-%d)\n", rank,
                rel_start_ix, rel_end_ix);
  relax(rel_start_ix, rel_end_ix, input_arr, output_arr);
  return rel_end_ix;
}

/*
 * Handles the delegation of work and then the subsequent reduction of both
 * the return array rows, as well as precision value to check for completion
 */
void run_master(void) {
  printf("\nSTARTING MASTER EXECUTION\n");
  printf("=========================\n\n");

  /* Branch for master and slave execution */

  while (true) {
    if (V) printf("MASTER: About to start working on [%d-%d)...\n",
                  p_data[ROOT_PROCESS].start_ix,
                  p_data[ROOT_PROCESS].end_ix    );

    // Only send out work if there are multiple processors
    if (v) printf("MASTER: Dispatching work!\n");
    send_matrix(arr, ROOT_PROCESS);

    int nrelaxed = process_relax(arr, new_working_arr);
    if (V) printf("MASTER: Ready to merge relaxed fragments\n");
    if (V) print_matrix(new_working_arr, nrelaxed, dim);

    // Calculate precision
    if (v) printf("MASTER: calculating precision\n");
    recalc_prec_arr( 0, nrelaxed, arr, new_working_arr, precision_arr );

    print_matrix(precision_arr, length, dim);
    // Copy back relaxed values
    memcpy(arr, new_working_arr, nrelaxed * sizeof(float));

    receive_matrix(arr, rank);

    float precs[nprocesses];
    precs[ROOT_PROCESS] = get_max(precision_arr, nrelaxed);

    collect_precision(precs);

    current_precision = get_max(precs, nprocesses);
    printf("MASTER: Global precision is %f.\n", current_precision);
    if ( is_finished(current_precision) && nprocesses != 0) {
      send_termination_signal(arr);
      return;
    }
  }
}

void run_slave(void) {
  while (true) {
    receive_matrix(working_arr, rank);
    /* The matrix will contain a signal if precision has been reached */
    if (iter_counter != 0) {
      // new_working_arr contains last iterations result
      if (contains_termination_signal(working_arr, new_working_arr)) {
        if (v) printf("SLAVE %d: Termination signal received, exiting!\n",
                      rank);
        break;
      }
    }
    int nrelaxed = process_relax(working_arr, new_working_arr);
    if (V) printf("SLAVE %d: Ready to return fragment\n", rank);
    if (V) print_matrix(new_working_arr, nrelaxed, dim);
    send_matrix(new_working_arr, rank);

    if (V) printf("SLAVE %d: calculating precision\n", rank);
    recalc_prec_arr(0, nrelaxed, working_arr, new_working_arr, precision_arr);
    current_precision = get_max(precision_arr, nrelaxed);

    return_precision(current_precision);
  }
}

/* Calculate useful values at startup */
void establish_gbls(void) {
  min_rows_per_process = dim / nprocesses;

  p_data = malloc(nprocesses * sizeof(struct process_data));

  /* Populate process_data structs */
  int i;
  for (i = 0; i < nprocesses; ++i) {
    calculate_work_area();
  }

  if (v) {
    printf(
      "SLAVE %d: Will work on rows [%d, %d)\n",
      rank,
      p_data[rank].start_row,
      p_data[rank].end_row    );
  }

  /* Each process needs space for its designated rows + the one above and the
     one below (These will not always be full, e.g. when sending the top and
     bottom rows.
     An additional array is needed to store the relaxed values
   */
  working_arr = malloc(dim * (p_data[rank].nrows + 2) * sizeof(float));
  new_working_arr = malloc(dim * (p_data[rank].nrows + 2) * sizeof(float));
  precision_arr = malloc(dim * (p_data[rank].nrows + 2) * sizeof(float));

  if (v) printf("Min rows per process: %d\n\n", min_rows_per_process);
}
