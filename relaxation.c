#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Globals */
int dim, length, nthreads, precision;
float *arr;

pthread_barrier_t barrier;

struct thread_info {
    pthread_t thread_id;
    int       thread_num;
    float     *input_arr;
    float     *output_arr;
};

struct thread_info *threads;

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

bool has_met_precision(float max_change, int precision)
{
    // max_change must be lower than the error margin
    float err_margin = pow(10, (float) -precision);
    return max_change < err_margin;
}

/*
 * Set the precision array to the difference between the old and new matrix
 * values
 */
void recalc_prec_arr(float *old_vals, float *new_vals, float *prec_arr)
{
    int i;

    for (i = 0; i < length; ++i)
    {
        prec_arr[i] = fabs(old_vals[i] - new_vals[i]);
    }
}

/* Checks for the highest value in the array and returns it, the highest value
 * is the lowest precision which is how precision is measured
 */
float get_max(float *precision_arr)
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

/*
 * Both params are square arrays in 1D form
 *
 * Sets each element of new_values to the average of the 4 neighbours of each
 * cell in arr
 */
void relax (float *arr, float *new_values)
{
    float right, left, above, below;
    int i;

    // Calculate new values
    for (i = 0; i < length; ++i)
    {
        // Copy over edge cells (no computation needed)
        if (is_edge_index(i, dim))
        {
            new_values[i] = arr[i];
        }
        // Set cell to the average of it's neighbours
        else
        {
            right = arr[i+1];
            left  = arr[i-1];
            above = arr[i-dim];
            below = arr[i+dim];

            new_values[i] = (right + left + above + below) / 4;
        }
    }
    //pthread_join(thread_id, NULL);
}

void solve (void)
{
    float *precision_arr = malloc(length * sizeof(int));
    float *new_values    = malloc(length * sizeof(int));
    float max_change;

    // Iterate until relaxed to given precision
    while (true)
    {
        // TODO for all nthreads
        pthread_create(
            &thread_info[0].thread_id, NULL,
            &relax,                    &threads[0]
        );
        relax(arr, new_values);

        // Update contents of precision array
        recalc_prec_arr(arr, new_values, precision_arr);
        max_change = get_max(precision_arr);

        // Inform user
        printf( "Max change: %.3f\nState of matrix:\n",
                max_change                              );

        print_matrix(arr);

        /* Check base condition - return if precision is high enough */
        if ( has_met_precision(max_change, precision) )
        {
            printf( "Precise to %d decimal place(s). Relaxation Complete!\n",
                    precision);
            return;
        }
        else
        {
            // Continue iteration
            memcpy(arr, new_values, length * sizeof(float));
        }
    }
}

int main (int argc, char *argv[])
{
    /* Command line args: array filename, dimension, nthreads, precision */
    char *filename = "matrices/binmatrix";
    nthreads = 8;
    precision = 1;

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
            precision = (int) strtof(optarg, NULL);
            break;

          default:
            printf("Invalid arg: %s\n", optarg);
            break;
        }
    }
    arr = read_array(filename); // set length and dim
    threads = malloc(nthreads * sizeof(struct thread_info));
    solve();

    return 0;
}
