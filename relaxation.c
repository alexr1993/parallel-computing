#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool v, V; //verbose
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
        if (v) printf("Finished relaxing elements %d to %d ",
                     thread.start_ix,
                     thread.end_ix                           );

        if (v) printf("(thread %d)\n", thread_num);

        // Update contents of precision array
        recalc_prec_arr( thread.start_ix,
                         thread.end_ix,
                         arr,
                         temp_arr,
                         precision_arr    );

        pthread_barrier_wait(&barrier); // Wait for full relaxation

        if (is_main_thread)
        {

            max_change = get_max(precision_arr); // TODO parallelise reduce

            // Inform user
            if (v) printf( "Max change: %f\n",
                           max_change                              );
            if (V) printf("State of matrix:\n");
            if (V) print_matrix(arr);

            /* Check base condition - return if precision is high enough */
            if ( has_met_precision(max_change, precision) )
            {
                printf("Relaxation Complete!\n");
                printf("Precise to %d decimal place(s).\n",  precision);
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
    if (v) printf( "Thread %d assigned elements [%d to %d)\n",
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
    if (v) printf("Min elements per thread: %d\n", min_elements_per_thread);

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
        if (v) printf("Created thread %d\n", i);

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
    if (v) printf("Initiated plain matrix.\n");
    if (v) printf("Matrix length: %d, dimension: %d\n", length, dim);
    if (v) print_matrix(arr);
}

int main (int argc, char *argv[])
{
    // Default cmd line args
    char *filename = NULL;
    dim = 50;
    nthreads       = 1;
    precision      = 1;
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
          // precision (in decimal places)
          case 'p':
            precision = atoi(optarg);
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
        length = dim * dim;
        init_plain_matrix();
    }

    temp_arr      = malloc(length * sizeof(int));
    precision_arr = malloc(length * sizeof(int));

    threads = malloc(nthreads * sizeof(struct thread_info));

    printf("Beginning relaxation using ");
    printf("threads: %d, precision: %d, dim: %d, file: %s\n...\n",
           nthreads,
           precision,
           dim,
           filename
    );

    start();
    return 0;
}
