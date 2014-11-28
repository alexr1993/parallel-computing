#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool v, V; //verbose
int dim, length, nthreads, min_elements_per_thread, iter_counter;
float precision;
float *arr, *new_values_arr, *precision_arr;

pthread_barrier_t barrier;

/* Contains a threads data, notably start_ix and end_ix to designate areas of
 * the arrays to work on
 */
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
 * Sequentially prints matrix
 */
void print_matrix(float *arr)
{
    printf("\n");
    int i;
    for (i = 0; i < length; ++i)
    {
        printf("%.3f ", arr[i]);
        if (i % dim == dim - 1)
        {
            printf("\n");
        }
    }
    printf("\n");
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
        if (v) printf("Matrix length: %d, dimension: %d\n", length, dim);
    }
    else
    {
        if (v) printf("Matrix is not square, length: %d\n", length);
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
    }

    fclose(input);
    if (v) printf("Successfully read matrix:\n");
    if (v) print_matrix(arr);

    return arr;
}

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

        if (v) print_matrix(arr);
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
                print_matrix(arr);
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
 * Accepts thread_info as an arg, or NULL if it's the main thread
 */
void solve (void *arg)
{

    bool is_main_thread = false;

    thread_info thread = *(thread_info *)arg;

    int thread_num = thread.thread_num;
    if (thread_num == 0) is_main_thread = true;


    // Iterate until relaxed to given precision
    while (true)
    {
        relax(thread.start_ix, thread.end_ix, arr, new_values_arr);

        if (v) printf(
            "Finished averaging elements %d to %d (thread %d)\n",
            thread.start_ix,
            thread.end_ix,
            thread_num
        );

        // Update contents of precision array
        recalc_prec_arr( thread.start_ix,
                         thread.end_ix,
                         arr,
                         new_values_arr,
                         precision_arr    );

        pthread_barrier_wait(&barrier); // Wait for full relaxation

        if (is_main_thread)
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
    if (v) printf( "Thread %d assigned elements [%d to %d)\n\n",
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

    if (V) printf("\nAssigning Work and Starting Parallel Section\n");
    if (V) printf("============================================\n\n");
    if (v) printf("Min elements per thread: %d\n\n", min_elements_per_thread);

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
        if (v) printf("Created thread %d\n\n", i);

    }

    // Finally utilise the main thread too
    solve((void *)&threads[0]);
}

/* Creates an array which is all 0s apart from the edges which are 1s */
void init_plain_matrix()
{
    arr = malloc(length * sizeof(float));
    int i;
    for (i = 0; i < length; ++i)
    {
        if (is_edge_index(i, dim))
        {
            arr[i] = 1;
        }
        else arr[i] = 0;
    }
    if (v) printf("Initiated plain matrix:\n");
    if (v) printf("Matrix length: %d, dimension: %d\n\n", length, dim);
    if (v) print_matrix(arr);
}

int main (int argc, char *argv[])
{
    // Default cmd line args
    char *filename = NULL;
    dim = 50;
    nthreads       = 1;
    precision      = 0.1;
    v = false;
    V = false;

    // Parse args
    int c = 0;
    while ( (c = getopt(argc, argv, "d::f::p::n::v::V::")) != -1 )
    {
        switch (c)
        {
          // dimensions
          case 'd':
            dim = atoi(optarg);
            break;
          // filename
          case 'f':
            filename = malloc(strlen(optarg) * sizeof(char));
            strncpy(filename, optarg, strlen(optarg));
            break;
          // precision
          case 'p':
            precision = atof(optarg);
            break;
          // nThreads
          case 'n':
            nthreads = atoi(optarg);
            break;
          // verbose
          case 'v':
            v = true;
            break;
          // very verbose
          case 'V':
            V = true;
            v = true;
            break;
          default:
            printf("Invalid arg: %s\n", optarg);
            break;
        }
    }

    if (filename)
    {
        arr = read_array(filename); // also sets "length" and "dim"
    }
    else
    {
        printf("Generating matrix as no filename given.\n\n");
        length = dim * dim;
        init_plain_matrix();
    }

    /* Initialise and allocate */
    iter_counter   = 0;
    new_values_arr = malloc(length * sizeof(int));
    precision_arr  = malloc(length * sizeof(int));

    threads = malloc(nthreads * sizeof(struct thread_info));

    printf("Beginning relaxation using ");
    printf("threads: %d, precision: %f, dim: %d, file: %s...\n\n",
           nthreads,
           precision,
           dim,
           filename
    );

    start();
    return 0;
}
