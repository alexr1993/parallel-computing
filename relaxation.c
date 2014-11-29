#include "relaxation.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mpi.h"
#include "util.h"

extern bool v, V; //verbose
extern int dim, length, nprocesses, min_elements_per_process, iter_counter;
extern float precision;
extern float *arr, *new_values_arr, *precision_arr;

struct process_data;

struct process_data *processes;

/*
 * Set the precision array to the difference between the old and new matrix
 * values
 */
void recalc_prec_arr( int start_ix,    int end_ix, float *old_vals,
                      float *new_vals, float *prec_arr              )
{
    int i;

    for (i = start_ix; i < end_ix; ++i)
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

/*
 * Both params are square arrays in 1D form
 *
 * Sets each element of new_values to the average of the 4 neighbours of each
 * cell in arr
 */
void relax (int start_ix, int end_ix, float *arr, float *new_values)
{
    float right, left, above, below;
    int i;

    // Calculate new values
    for (i = start_ix; i < end_ix; ++i)
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
}

/*
 * Contains the sequential precision verification logic
 */
bool is_finished(float max_change)
{
    // Inform user
    if (v)
    {
        printf("\nIteration %d Complete - Checking Exit Condition\n",
               iter_counter);
        if (V)
            printf("============================================\n\n");

        printf( "\nMax change: %f (Precision requested: %f)\n",
                max_change, precision );

    }

    /* Check base condition - return true if precision is high enough */
    if (max_change < precision)
    {
        if (!v) // prevent informing twice in verbose mode
            printf("Max change: %f\n", max_change);

        printf("\nRelaxation Complete (%d Iterations)!\n",
               iter_counter);

        if (v) print_matrix(arr, length, dim);
        return true;
    }
    else
    {
        /* Verboseness */
        if (v)
        {
            if (V)
            {
                printf("\nState of matrix:\n");
                print_matrix(arr, length, dim);
            }
            printf("\nPrecision Not Reached - Iterating\n");
            if (V) printf("=================================\n");
            printf("\n");
        }
        return false;
    }
}

/*
 * Iteratively relaxes the array until precision is met
 *
 * Accepts process_data as an arg, or NULL if it's the main process
 */
void solve (void *arg)
{

    bool is_main_process = false;

    process_data process = *(process_data *)arg;

    int process_num = process.process_num;
    if (process_num == 0) is_main_process = true;


    // Iterate until relaxed to given precision
    while (true)
    {
        relax(process.start_ix, process.end_ix, arr, new_values_arr);

        if (v) printf(
            "Finished averaging elements %d to %d (process %d)\n",
            process.start_ix,
            process.end_ix,
            process_num
        );

        // Update contents of precision array
        recalc_prec_arr( process.start_ix,
                         process.end_ix,
                         arr,
                         new_values_arr,
                         precision_arr    );

        if (is_main_process)
        {
            ++iter_counter;

            // Put the new values into the main array
            // This could be parallelised
            memcpy(arr, new_values_arr, length * sizeof(float));

            if ( is_finished(get_max(precision_arr)) )
            {
                return;
            }
        }
    }
}
