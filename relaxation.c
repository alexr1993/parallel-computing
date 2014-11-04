#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/*
 * Reads square array of length dim from file. Ignores whitespace/newlines
 *
 * Input matrices must contain only digits, non tab spaces, and newliens
 */
float *read_array(char *filename)
{
    // Infer dimension and check validity or matrix
    FILE *input = fopen(filename, "r");
    int character = fgetc(input);
    length = 0;

    while (character != EOF)
    {
        if ( !(character == ' ' || character == '\n'))
        {
            ++length;
        }
        character = fgetc(input);
    }

    // Check array is square
    double root_of_length = sqrt((double)length);
    if (floor(root_of_length) == root_of_length)
    {
        dim = (int)root_of_length;
        printf("Matrix length: %d, dimension: %d\n", length, dim);
    }
    else
    {
        printf("Matrix is not square, length: %d\n", length);
        exit(1);
    }
    fclose(input);

    // Deserialize array
    arr = malloc(length * sizeof(int));
    int i;
    input = fopen(filename, "r"); // reopen file to reset ptr

    int temp;

    for (i = 0; i < length; ++i)
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

void relax (float *arr, float *temp_arr)
{
    float right, left, above, below;
    int i;

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
}

void solve (float *arr, int dim, int nthreads, float precision)
{
    int i;
    int length = dim * dim;

    float *precision_arr = malloc(length * sizeof(int));
    float *temp_arr      = malloc(length * sizeof(int));

    // Initiliase current_precision to unsatisfactory value
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
                precision_arr[i] = calc_precision( cell, right, left,
                                                   above, below       );
            }
        }

        current_precision = get_current_precision(precision_arr, length);
        printf("Current precision: %.3f\nState of matrix:\n",
               current_precision);
        print_matrix(arr);

        /* Check base condition - return if precision is high enough */
        if (current_precision < precision)
        {
            printf("Relaxation Complete!\n");
            printf("Precision: %.3f\n", current_precision);
            return;
        }

        // Continue iteration
        relax(arr, temp_arr);
        memcpy(arr, temp_arr, length * sizeof(float));
    }
}

int main (int argc, char *argv[])
{
    /* Command line args: array filename, dimension, nthreads, precision */
    char *filename = "matrices/matrix";
    int nthreads = 8;
    float precision = 7;

    int c = 0;

    while ((c = getopt(argc, argv, "f:p:d:")) != -1)
    {
        switch (c)
        {
          case 'f':
            filename = malloc(strlen(optarg) * sizeof(char));
            strncpy(filename, optarg, strlen(optarg));
            printf("Reading matrix from file: %s\n", filename);
            break;

          case 'p':
            precision = strtof(optarg, NULL);
            break;

          default:
            printf("Invalid arg: %s\n", optarg);
            break;
        }
    }
    arr = read_array(filename); // set length and dim

    solve(arr, dim, nthreads, precision);

    return 0;
}
