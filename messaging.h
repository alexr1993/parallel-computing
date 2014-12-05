#ifndef MESAGING_H_
#define MESSAGING_H_

#define ROOT_PROCESS 0
void send_matrix(float *data, int rank);
void receive_matrix(float *data, int rank);
void collect_precision(float *prec);
void return_precision(float prec);
#define send_data_tag 2001
#define return_data_tag 2002
#define ROOT_PROCESS 0

#endif
