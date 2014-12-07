#ifndef MESAGING_H_
#define MESSAGING_H_

#include <stdbool.h>

#define ROOT_PROCESS 0

#define send_data_tag 2001
#define return_data_tag 2002

void send_matrix(float *data, int rank);
void receive_matrix(float *data, int rank);
void collect_precision(float *prec);
void return_precision(float prec);
void send_termination_signal(float *data);
bool contains_termination_signal(float * data, float *prev_data);

#endif
