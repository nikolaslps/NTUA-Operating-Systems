#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
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

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */

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

int thrcnt;//Globalizing the total thread count
sem_t *semaphores;//Globalizing sempaphore array

void *compute_and_output_mandel_line(void *arg)
{
        /*
         * A temporary array, used to hold color values for the line being drawn
         */
        struct thread_info_struct *thread = arg;

	 int color_val[x_chars];
        for (int i = thread->thread_number; i < y_chars; i += thrcnt){
	//each thread increases it's iterator by thread_count 
	//Redisistributing the work in a cyclic manner


        compute_mandel_line(i, color_val);

	int number = thread->thread_number;
        if (sem_wait(&semaphores[number]) != 0){
                perror("sem_wait");
                exit(1);
        }

        output_mandel_line(1, color_val);

        if (sem_post(&semaphores[(number + 1) % thrcnt]) != 0){
                perror("sem_signal");
        }
        }
        return NULL;
}

int main(int argc, char *argv[])
{
        if (argc != 2){
                perror("We need a positive thread couny");
                exit(1);
        }

        if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0){
                fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
                exit(1);
        }

        semaphores = (sem_t*)safe_malloc(thrcnt * sizeof(sem_t));//dynamic memory for semaphore array

	 if (sem_init(&semaphores[0], 0, 1) != 0){//initializing first semaphore to one
                perror("sem init");
                exit(1);
        }

        for (int i = 1; i < thrcnt; i++){//initializing the rest semaphores to 0
                if (sem_init(&semaphores[i], 0, 0) != 0){
                perror("sem init");
                exit(1);
	}
        }

        struct thread_info_struct *thr;
        thr = safe_malloc(thrcnt * sizeof(*thr));//dynamic allocation of memory for array of thread inforamtion
        int ret;

        xstep = (xmax - xmin) / x_chars;
        ystep = (ymax - ymin) / y_chars;

        /*
         * draw the Mandelbrot Set, one line at a time.
         * Output is sent to file descriptor '1', i.e., standard output.
         */

        for (int i = 0; i < thrcnt; i++){
		thr[i].thread_number = i;//passing the number of the thread on the struct array
                ret = pthread_create(&thr[i].tid, NULL, compute_and_output_mandel_line, &thr[i]);
                if (ret){
                        perror("thread create");
                        exit(1);
                }
        }

        for (int i = 0; i < thrcnt; i++){//waiting for threads
                ret = pthread_join(thr[i].tid, NULL);
                if (ret){
                        perror ("thread join");
                        exit(1);
                }
        }

        for (int i = 0; i < thrcnt; i++){
                if (sem_destroy(&semaphores[i]) != 0){
                        perror("sema destruction");
                        exit(1);
                }
        }

        free (semaphores);
	free (thr);
        reset_xterm_color(1);
        return 0;
}
