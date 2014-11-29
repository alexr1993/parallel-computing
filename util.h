#ifndef UTIL_H_
#define UTIL_H_
void print_matrix(float *arr, int length, int dim);
bool is_edge_index (int index, int dim);
void validate_array(char *filename, int *length, int *dim);
float *read_array(char *filename, int *length, int *dim);
#endif
