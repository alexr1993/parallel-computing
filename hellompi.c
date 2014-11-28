#include "mpi.h"
#include <stdio.h>

int main( int argc, char *argv[] ) {
    int rank, size, rc;

    rc = MPI_Init(&argc, &argv); // Starts MPI

    if (rc != MPI_SUCCESS) {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }


    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get current process id
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Get number of processes

    printf("Hello world from process %d of %d\n", rank, size);
    MPI_Finalize();
    return 0;
}
