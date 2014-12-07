#include "util.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "messaging.h"

extern bool v, V;
extern int rank;
extern int nprocesses;

/*
 * Sequentially prints matrix (may be a fragment, determined by length)
 */
void print_matrix(float *arr, int length, int dim) {
  printf("\n");
  int i;
  for (i = 0; i < length; ++i) {

    printf("%.3f ", arr[i]);
    if (i % dim == dim - 1) {
      printf("\n");
    }
  }
  printf("\n");
}

bool is_on_edge_row(int index, int length, int dim) {
  // Check if index is on first or last row
  if (index < dim || index >= length - dim) {
      return true;
  }
  return false;
}

bool is_on_edge_column(int index, int dim) {
  // Check if index is on left or right
  int mod = (index % dim);
  return mod == 0 || mod == dim - 1;
}

// Uses 1D array index to check if the cell would lie on the edge of the
// square array
bool is_edge_index (int index, int length, int dim)
{
  return is_on_edge_row(index, length, dim) || is_on_edge_column(index, dim);
}

/*
 * Checks array is square and sets the global length and dim vars
 *
 */
void validate_array(char *filename, int *length, int *dim) {
  // Determine length
  FILE *input = fopen(filename, "r");
  int character = fgetc(input);

  if (V) printf("PROCESS %d: Calculating array length.\n", rank);
  int len = 0;
  while (character != EOF) {
    // Count anything that's not a space
    if ( !(character == ' ' || character == '\n')) {
      ++len;
    }
    character = fgetc(input);
  }
  fclose(input);

  if (V) printf("PROCESS %d: Checking array is square.\n", rank);
  // Check array is square
  double root_of_length = sqrt((double)len);
  if (floor(root_of_length) == root_of_length) {
    *dim = (int)root_of_length;
    *length = len;
    if (v) printf("Matrix length: %d, dimension: %d\n", len, *dim);
  } else {
    if (v) printf("Matrix is not square, length: %d\n", len);
    exit(1);
  }
  return;
}

/*
 * Reads square array of length dim from file. Ignores whitespace/newlines
 *
 * Input matrices must contain only digits, non tab spaces, and newliens
 */
float *read_array(char *filename, int *length, int *dim) {
  if (V) printf("PROCESS %d: Reading file: %s.\n", rank, filename);

  // Infer dimension and check validity or matrix
  validate_array(filename, length, dim);

  // Deserialize array
  float *arr = malloc(*length * sizeof(float));
  int i;
  FILE *input = fopen(filename, "r");

  int temp;

  for (i = 0; i < *length; ++i) {
    // Skip whitespace
    do {
      temp = fgetc(input);
    } while (temp == ' ' || temp == '\n');

    // Store number
    arr[i] = temp - '0'; // Coerce char to int
  }

  fclose(input);
  if (v) printf("PROCESS %d: Successfully read matrix:\n", rank);
  if (v) print_matrix(arr, *length, *dim);

  return arr;
}

/* Creates an array which is all 0s apart from the edges which are 1s */
float *create_plain_matrix(int length, int dim) {
  float *arr = malloc(length * sizeof(float));

  int i;
  for (i = 0; i < length; ++i) {
    // Set the edges to 1 and everything else to 0
    if (is_edge_index(i, length, dim)) {
      arr[i] = 1;
    } else {
      arr[i] = 0;
    }
  }
  if (v) printf("PROCESS %d: Initiated plain matrix:\n", rank);
  if (v) printf("Matrix length: %d, dimension: %d\n\n", length, dim);
  if (v) print_matrix(arr, length, dim);

  return arr;
}

/* Returns largest value in array - for precision check */
float get_max(float *arr, int len) {
  int i;
  float max = -1;

  for (i = 0; i < len; ++i) {
    if (arr[i] > max) max = arr[i];
  }
  return max;
}

/* Governs whether the process needs 1 or 2 extra rows of padding to perform
 * relaxation
 */
int get_nrows_of_padding(void) {
  // 1 process for whole array
  if (nprocesses == 1) {
    return 0;
  // Process has the top or bottom row instead of padding
  } else if (rank == ROOT_PROCESS || rank == nprocesses -1) {
    return 1;
  // Process needs padding for both top and bottom
  } else {
    return 2;
  }
}
