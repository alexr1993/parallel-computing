#ifndef UTIL_H_
#define UTIL_H_
void print_matrix(float *arr, int length, int dim);
bool is_edge_index (int index, int length, int dim);
float *read_array(char *filename, int *length, int *dim);
float *create_plain_matrix(int length, int dim);
float get_max(float *arr, int len);
#endif
