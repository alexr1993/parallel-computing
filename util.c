#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern bool v, V;

/*
 * Sequentially prints matrix
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

// Uses 1D array index to check if the cell would lie on the edge of the
// square array
bool is_edge_index (int index, int dim)
{
  // Check if index is on first or last row
  if (index < dim || index >= dim * dim - dim) {
      return true;
  }

  // Check if index is on left or right
  int mod = (index % dim);
  return mod == 0 || mod == dim - 1;
}

/*
 * Checks array is square and sets the global length and dim vars
 *
 */
void validate_array(char *filename, int *length, int *dim) {
  // Determine length
  FILE *input = fopen(filename, "r");
  int character = fgetc(input);

  int len = 0;
  while (character != EOF) {
    // Count anything that's not a space
    if ( !(character == ' ' || character == '\n')) {
      ++len;
    }
    character = fgetc(input);
  }
  fclose(input);

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
float *read_array(float *arr, char *filename, int *length, int *dim) {

  // Infer dimension and check validity or matrix
  validate_array(filename, length, dim);

  FILE *input;

  // Deserialize array
  arr = malloc(*length * sizeof(float));
  int i;
  input = fopen(filename, "r"); // reopen file to reset ptr

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
  if (v) printf("Successfully read matrix:\n");
  if (v) print_matrix(arr, *length, *dim);

  return arr;
}

/* Creates an array which is all 0s apart from the edges which are 1s */
float *create_plain_matrix(int length, int dim) {
  float *arr = malloc(length * sizeof(float));

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

  return arr;
}


