#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dim;
int length;
float *arr;

void print_matrix(float *arr)
{
    int i;
    for (i = 0; i < length; ++i)
    {
        printf("%.3f ", arr[i]);
        if (i % dim == dim - 1)
        {
            printf("\n");
        }
    }
    printf("\n\n");
}

/* Reads square array of length dim from file. Ignores whitespace/newlines */
float *read_array(char *filename, int dim)
{
    int nelements = dim * dim;
    arr = malloc(nelements * sizeof(int));
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
        //printf("%.3f\n", arr[i]);
    }

    fclose(input);
    return arr;
}

/*
 * Returns true if the difference between cell and it's neighbours is less
 * than prec
 */
float calc_precision (float cell, float right, float left,
                      float above, float below            )
{
    float rightd = fabs(cell - right);
    float leftd  = fabs(cell - left);
    float aboved = fabs(cell - above);
    float belowd = fabs(cell - below);

    float prec_arr[4] = { rightd, leftd, aboved, belowd };

    float max_precision = 0;
    int i;

    for (i = 0; i < 4; ++i)
    {
        if (prec_arr[i] > max_precision)
        {
            max_precision = prec_arr[i];
        }
    }
    return max_precision;
}

/* Checks for the highest value in the array and returns it, the highest value
 * is the lowest precision which is how precision is measured
 */
float get_current_precision(float *precision_arr, int length)
{
    int i;
    float max = 0;
    float cell_prec;

    for (i = 0; i < length; ++i)
    {
        cell_prec = precision_arr[i];
        // Return false if cell is not precise enough
        if (cell_prec > max)
        {
            max = cell_prec;
        }
    }
    return max;
}

// Uses 1D array index to check if the cell would lie on the edge of the
// square array
bool is_edge_index (int index, int dim)
{
    // Check if index is on first or last row
    if (index < dim || index >= dim * dim - dim)
    {
        return true;
    }

    // Check if index is on left or right
    int mod = (index % dim);
    return mod == 0 || mod == dim - 1;
}

void solve (float *arr, int dim, int nthreads, float precision)
{
    int i;
    int length = dim * dim;

    float *precision_arr = malloc(length * sizeof(int));

    /* Not precise enough yet - Relax */
    float *temp_arr = malloc(length * sizeof(int));

    // Initiliase to get inside loop
    float current_precision = precision + 1.0;

    float right, left, above, below, cell;

    // Iterate
    while (true)
    {
        // Calculate current precision
        for (i = 0; i < length; ++i)
        {
            cell  = arr[i];
            right = arr[i+1];
            left  = arr[i-1];
            above = arr[i-dim];
            below = arr[i+dim];

            // Can only calculate precision for interior cells
            if (is_edge_index(i, dim))
            {
                // Nothing to do here
            }
            else
            {
                precision_arr[i] = calc_precision(cell, right, left, above, below);
            }
        }

        current_precision = get_current_precision(precision_arr, length);

        /* Check base condition - return if precision is high enough */
        if (current_precision < precision)
        {
            printf("Relaxation Complete!\nPrecision: %.3f\n", current_precision);
            return;
        }

        // Calculate new values
        for (i = 0; i < length; ++i)
        {
            // Copy over edge cells (no computation needed)
            if (is_edge_index(i, dim))
            {
                temp_arr[i] = arr[i];
            }
            // Set cell to the average of it's neighbours
            else
            {
                right = arr[i+1];
                left  = arr[i-1];
                above = arr[i-dim];
                below = arr[i+dim];

                temp_arr[i] = (right + left + above + below) / 4;
            }
        }
        // Copy over results
        memcpy(arr, temp_arr, length * sizeof(float));
        printf("Current precision: %.3f\nState of matrix:\n",
               current_precision);
        print_matrix(temp_arr);
    }
}

int main (int argc, char *argv[])
{
    /* Command line args: array filename, dimension, nthreads, precision */
    char *filename = "matrix";
    dim = 4;
    length = dim * dim;
    int nthreads = 8;
    float precision = 7;
    arr = read_array(filename, dim);

    solve(arr, dim, nthreads, precision);

    return 0;
}
