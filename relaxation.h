#ifndef RELAXATION_H_
#define RELAXATION_H_

#include <stdbool.h>
bool is_finished(float max_change);
void relax (int start_ix, int end_ix, float *arr, float *new_values);
void recalc_prec_arr( int start_ix,    int end_ix, float *old_vals,
                      float *new_vals, float *prec_arr              );
#endif
