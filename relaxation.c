#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int *read_array(char *filename, int dim)
{
    int nelements = dim * dim;
    int *arr = malloc(nelements * sizeof(int));
    int i;
    FILE *input = fopen(filename, "r");

    int temp;

    for (i = 0; i < nelements; ++i)
    {
        // Skip whitespace
        do {
            temp = fgetc(input);
        } while (temp == ' ' || temp == '\n');

        // Store number
        arr[i] = temp - '0'; // Coerce char to int
        printf("%d\n", arr[i]);
    }

    fclose(input);
    return arr;
}

/*
 * Returns true if the difference between cell and it's neighbours is less
 * than prec
 */
bool is_precise_enough (int prec, int cell, int right,
                        int left, int above, int below )
{
    return prec > abs(cell - right) && prec > abs(cell - left)
        && prec > abs(cell - above) && prec > abs(cell - below);
}

void solve (int *arr, int dim, int nthreads, float precision)
{
    int i;
    int right, left, above, below, cell;

    for (i = 0; i < dim; ++i)
    {
        cell  = arr[i];
        right = arr[i+1];
        left  = arr[i-1];
        above = arr[i-dim];
        below = arr[i+dim];

        if (is_precise_enough(precision, cell, right, left, above, below))
        {
            printf("Relaxation Complete!\n");
            return;
        }
    }
    return;
}

int main (int argc, char *argv[])
{
    /* Command line args: array filename, dimension, nthreads, precision */
    char *filename = "matrix";
    int dim = 4;
    int nthreads = 8;
    float precision = 100.2;
    int *arr = read_array(filename, dim);

    solve(arr, dim, nthreads, precision);

    return 0;
}
