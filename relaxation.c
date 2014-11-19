#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Globals */
int dim, length, nthreads, precision, min_elements_per_thread;
float *arr, *temp_arr, *precision_arr;

pthread_barrier_t barrier;

typedef struct thread_info {
    pthread_t thread_id;
    int       thread_num;
    int       start_ix;
    int       end_ix;
    float     *input_arr;
    float     *output_arr;
} thread_info;

struct thread_info *threads;

/*
 *
 */
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
 * Checks array is square and sets the global length and dim vars
 *
 */
void validate_array(char *filename)
{
    // Determine length
    FILE *input = fopen(filename, "r");
    int character = fgetc(input);

    length = 0;
    while (character != EOF)
    {
        // Count anything that's not a space
        if ( !(character == ' ' || character == '\n'))
        {
            ++length;
        }
        character = fgetc(input);
    }
    fclose(input);

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
    return;
}

/*
 * Reads square array of length dim from file. Ignores whitespace/newlines
 *
 * Input matrices must contain only digits, non tab spaces, and newliens
 */
float *read_array(char *filename)
{
    validate_array(filename);

    // Infer dimension and check validity or matrix
    FILE *input;

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
    printf("Successfully read matrix:\n");
    print_matrix(arr);

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
    //pthread_join(thread_id, NULL);
}


/*
 * Iteratively relaxes the array until precision is met
 *
 * Accepts thread_info as an arg, or NULL if it's the main thread
 */
void solve (void *arg)
{

    bool is_main_thread = false;

    thread_info thread = *(thread_info *)arg;

    int thread_num = thread.thread_num;
    if (thread_num == 0) is_main_thread = true;

    float max_change;

    // Iterate until relaxed to given precision
    while (true)
    {
        relax(thread.start_ix, thread.end_ix, arr, temp_arr);
        printf("Finished relaxing elements %d to %d ",
               thread.start_ix,
               thread.end_ix                           );

        printf("(thread %d)\n", thread_num);

        pthread_barrier_wait(&barrier); // Wait for full relaxation

        if (is_main_thread)
        {
            // Update contents of precision array
            recalc_prec_arr(arr, temp_arr, precision_arr);
            max_change = get_max(precision_arr);

            // Inform user
            printf( "Max change: %.3f\nState of matrix:\n",
                    max_change                              );
            print_matrix(arr);

            /* Check base condition - return if precision is high enough */
            if ( has_met_precision(max_change, precision) )
            {
                printf("Precise to %d decimal place(s). ",  precision);
                printf("Relaxation Complete!\n");
                return;
            }
            else
            {
                // Continue iteration
                memcpy(arr, temp_arr, length * sizeof(float));
            }
        }
        pthread_barrier_wait(&barrier);
    }
}

/*
 * Marks thread info with the elements it must crunch
 */
void assign_work(thread_info *thread)
{
    thread->start_ix = thread->thread_num * min_elements_per_thread;
    thread->end_ix = thread->start_ix + min_elements_per_thread;

    // The last thread picks up the remainder
    if (thread->thread_num == nthreads - 1)
    {
        thread->end_ix += length % nthreads;
    }
    printf( "Thread %d assigned elements [%d to %d)\n",
            thread->thread_num,
            thread->start_ix,
            thread->end_ix
    );

    return;
}

/*
 * Creates n - 1 child threads then passes the main one through
 */
void start(void)
{
    pthread_barrier_init( &barrier, NULL, nthreads );

    min_elements_per_thread = length / nthreads;
    printf("Min elements per thread: %d\n", min_elements_per_thread);

    int i;
    assign_work(&threads[0]); // Give main thread first chunk

    // spawn threads 1 to n - 1, 0 is reserved for the main thread
    for (i = 1; i < nthreads; ++i)
    {
        threads[i].thread_num = i;
        assign_work(&threads[i]);
        pthread_create( &threads[i].thread_id,
                        NULL,
                        (void *(*)(void *))&solve,
                        &threads[i]                );
        printf("Created thread %d\n", i);

    }

    // Finally utilise the main thread too
    printf("%d and %d\n", threads[0].start_ix, threads[0].end_ix);
    solve((void *)&threads[0]);
}

int main (int argc, char *argv[])
{
    // Default cmd line args
    char *filename = "matrices/binmatrix";
    nthreads       = 1;
    precision      = 1;

    // Parse args
    int c = 0;
    while ( (c = getopt(argc, argv, "f::p::n::")) != -1 )
    {
        switch (c)
        {
          // filename
          case 'f':
            filename = malloc(strlen(optarg) * sizeof(char));
            strncpy(filename, optarg, strlen(optarg));
            printf("Reading matrix from file: %s\n", filename);
            break;
          // precision (in decimal places)
          case 'p':
            precision = atoi(optarg);
            break;
          // nThreads
          case 'n':
            nthreads = atoi(optarg);
            break;
          default:
            printf("Invalid arg: %s\n", optarg);
            break;
        }
    }

    arr           = read_array(filename); // also sets "length" and "dim"
    temp_arr      = malloc(length * sizeof(int));
    precision_arr = malloc(length * sizeof(int));

    threads = malloc(nthreads * sizeof(struct thread_info));

    printf("Beginning relaxation using ");
    printf("threads: %d, precision: %d, file: %s\n",
           nthreads,
           precision,
           filename
    );

    start();
    return 0;
}