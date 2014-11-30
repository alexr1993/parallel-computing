#ifndef MESAGING_H_
#define MESSAGING_H_

#define ROOT_PROCESS 0
void send_matrix(int dim, float *data, int rank);
float *receive_matrix(int *length, int *dim);
void send_process_data(int start_ix, int end_ix, int rank);
void receive_process_data(int *start_ix_ptr, int *end_ix_ptr);

#endif
