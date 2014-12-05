#ifndef MAIN_H_
#define MAIN_H_

/* This struct allows the master to know how to split up and reduce work */
typedef struct process_data {
  int start_row;
  int end_row;
  int nrows;
  int start_ix;
  int end_ix;
  int nelements;
} process_data;

#endif
