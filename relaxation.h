#ifndef RELAXATION_H_
#define RELAXATION_H_

#include <stdbool.h>
bool is_finished(float max_change);
void relax (int start_ix, int end_ix, float *arr, float *new_values);
#endif
