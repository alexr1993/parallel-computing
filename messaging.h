#ifndef MESAGING_H_
#define MESSAGING_H_

#define ROOT_PROCESS 0 
void send_matrix(int dim, float *data, int rank);
float *receive_matrix(int *length, int *dim);
#endif
