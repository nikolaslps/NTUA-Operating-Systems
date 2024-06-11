#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

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

struct thread_info_struct {
        pthread_t tid; /* POSIX thread id, as returned by the library */
        int thread_number;
};


int safe_atoi(char *s, int *val)
{
        long l;
        char *endp;

        l = strtol(s, &endp, 10);
        if (s != endp && *endp == '\0') {
                *val = l;
                return 0;
        } else
                return -1;
}

void *safe_malloc(size_t size)
{
        void *p;

        if ((p = malloc(size)) == NULL) {
                fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
                        size);
                exit(1);
        }

        return p;
}

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
        for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

                /* Compute the point's color value */
                val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
                if (val > 255)
                        val = 255;

                /* And store it in the color_val[] array */
                val = xterm_color(val);
                color_val[n] = val;
        }
}


void output_mandel_line(int fd, int color_val[])
{
        int i;
        
        char point ='@';
        char newline='\n';

        for (i = 0; i < x_chars; i++) {
                /* Set the current color, then output the point */
                set_xterm_color(fd, color_val[i]);
                if (write(fd, &point, 1) != 1) {
                        perror("compute_and_output_mandel_line: write point");
                        exit(1);
                }
        }

        /* Now that the line is done, output a newline character */
        if (write(fd, &newline, 1) != 1) {
                perror("compute_and_output_mandel_line: write newline");
                exit(1);
        }
}


pthread_mutex_t mutex;
pthread_cond_t cond;  
int next_line = 0;
int thrcnt;

void *compute_and_output_mandel_line(void *arg) {
    struct thread_info_struct *thread = arg;
    int color_val[x_chars];

    for (int i = thread->thread_number; i < y_chars; i += thrcnt) {
        compute_mandel_line(i, color_val);  // Compute the line first without locking.

        pthread_mutex_lock(&mutex);//This lock ensures that wait is used correctly

        // Wait until it's this thread's turn to output.
        while (i != next_line) {
            pthread_cond_wait(&cond, &mutex);
        }

        // Output the line as it's now this thread's turn.
        output_mandel_line(1, color_val);
        next_line++;  // next_line is common for all threads
        pthread_cond_broadcast(&cond);  // Wake up all thread waiting on cond

        pthread_mutex_unlock(&mutex); 
   }
    return NULL;
}


int main (int argc, char **argv)
{
        if (argc != 2){
                perror("We need a positive thread couny");
                exit(1);
        }

        if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0){
                fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
                exit(1);
        }


    int ret;     
    struct thread_info_struct *thr = safe_malloc(thrcnt * sizeof(*thr));

   if (pthread_mutex_init(&mutex, NULL)){
                perror("mutex");
                exit(1);
}

       if (pthread_cond_init(&cond, NULL) != 0){
                perror("condition-variables");
                exit(1);
        }
    

     xstep = (xmax - xmin) / x_chars;
        ystep = (ymax - ymin) / y_chars;

        /*
         * draw the Mandelbrot Set, one line at a time.
         * Output is sent to file descriptor '1', i.e., standard output.
         */

        for (int i = 0; i < thrcnt; i++){
                thr[i].thread_number = i;
                ret = pthread_create(&thr[i].tid, NULL, compute_and_output_mandel_line, &thr[i]);
                if (ret){
                        perror("thread create");
                        exit(1);
                }
        }


                for (int i = 0; i < thrcnt; i++){
                ret = pthread_join(thr[i].tid, NULL);
                if (ret){
                        perror ("thread join");
                        exit(1);
                }
        }

         for (int i = 0; i < thrcnt; i++) 
       if (pthread_cond_destroy(&cond) != 0){
                perror("condition variables");
                exit(1);
                }
    
  if (pthread_mutex_destroy(&mutex) != 0){
                perror("mutex");
                exit(1);
        }

    free(thr);

    reset_xterm_color(1);
    return 0;
}
