/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

// #define NTHREADS 3
int thrcnt; // Globalizing the total thread count

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int next_line = 0;

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
 */
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
 */
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

// struct thread_info_struct
// {
//     pthread_t tid; /* POSIX thread id, as returned by the library */
//     int thread_number;
// };

int safe_atoi(char *s, int *val)
{
    long l;
    char *endp;

    l = strtol(s, &endp, 10);
    if (s != endp && *endp == '\0')
    {
        *val = l;
        return 0;
    }
    else
        return -1;
}

void *safe_malloc(size_t size)
{
    void *p;

    if ((p = malloc(size)) == NULL)
    {
        fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
                size);
        exit(1);
    }

    return p;
}

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
    /*
     * x and y traverse the complex plane.
     */
    double x, y;

    int n;
    int val;

    /* Find out the y value corresponding to this line */
    y = ymax - ystep * line;

    /* and iterate for all points on this line */
    for (x = xmin, n = 0; n < x_chars; x += xstep, n++)
    {

        /* Compute the point's color value */
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255)
            val = 255;

        /* And store it in the color_val[] array */
        val = xterm_color(val);
        color_val[n] = val;
    }
}

int current_line = 0; // Global variable to keep track of the current line being printed

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
    int i;
    char point = '@';
    char newline = '\n';

    for (i = 0; i < x_chars; i++)
    {
        /* Set the current color, then output the point */
        set_xterm_color(fd, color_val[i]);
        if (write(fd, &point, 1) != 1)
        {
            perror("compute_and_output_mandel_line: write point");
            exit(1);
        }
    }

    /* Now that the line is done, output a newline character */
    if (write(fd, &newline, 1) != 1)
    {
        perror("compute_and_output_mandel_line: write newline");
        exit(1);
    }
    current_line++;
    pthread_cond_broadcast(&cond);
    //pthread_cond_signal(&cond);
    // notify with cond signal that output is ready
}

void *compute_and_output_mandel_line(void *arg)
{
    // pointer to the file descriptor that is responsible for the output
    int *fd = (int *)arg;
    int line = 0;

    while (1) // run until a condition inside tells you to stop
    {
        pthread_mutex_lock(&mutex);
        line = next_line;
        next_line = (next_line + 1) % y_chars; // circular sharing of work

        //Check if we have processed all lines
        if (line >= y_chars - 1)
        {
            pthread_mutex_unlock(&mutex);
            pthread_cond_wait(&cond, &mutex);
            exit(0);
        }

        pthread_mutex_unlock(&mutex);

        //printf("Thread %ld processing line %d\n", pthread_self(), line);

        /*
         * A temporary array, used to hold color values for the line being drawn
         */
        int color_val[x_chars];

        compute_mandel_line(line, color_val);

        // lock so that the output is in the correct order
        pthread_mutex_lock(&mutex);
        // Wait until it's this thread's turn to print
        while (current_line != line)
        {   
            //printf("now in line %d\n", current_line);
            pthread_cond_wait(&cond, &mutex);
        }

        output_mandel_line(*fd, color_val);
        pthread_cond_wait(&cond, &mutex);
        
        // unlock when the correct line is printed
        pthread_mutex_unlock(&mutex);
        
    }
    //pthread_cond_signal(&cond);
    pthread_exit(NULL);
    // return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        perror("We need a positive argument that indicates how many threads to create");
        exit(1);
    }

    if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0)
    {
        fprintf(stderr, "'%s' is not valid for 'thread_count'\n", argv[1]);
        exit(1);
    }

    int fd = 1; // Standart output

    // struct thread_info_struct *th;
    // th = safe_malloc(thrcnt * sizeof(*th));
    pthread_t th[thrcnt];

    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    // for debugging
    // int total_lines = y_chars;
    // printf("Total lines to process: %d\n", total_lines);

    /*
     * draw the Mandelbrot Set, one line at a time.
     * Output is sent to file descriptor '1', i.e., standard output.
     */
    pthread_mutex_init(&mutex, NULL);
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        perror("pthread_mutex_init");
        exit(1);
    }

    pthread_cond_init(&cond, NULL);
    if (pthread_cond_init(&cond, NULL) != 0)
    {
        perror("pthread_cond_init");
        pthread_mutex_destroy(&mutex);
        exit(1);
    }

    for (int i = 0; i < thrcnt; i++)
    {
        if (pthread_create(&th[i], NULL, &compute_and_output_mandel_line, &fd) != 0)
        {
            perror("pthread_create");
            exit(1);
        }
        printf("Thread %d has started\n", i);

        // if (pthread_join(th[i], NULL) != 0)
        // {
        //     exit(1);
        // }
        // printf("Thread %d has finished execution\n", i);
    }
    for (int i = 0; i < thrcnt; i++)
    {
        if (pthread_join(th[i], NULL) != 0)
        {
            exit(1);
        }
        printf("Thread %d has finished execution\n", i);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    reset_xterm_color(1);
    exit(0);
}

