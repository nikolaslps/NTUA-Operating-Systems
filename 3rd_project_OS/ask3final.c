#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdint.h>

#define P 5 //number of child processes

int num_processes = 0;

sem_t semaphore; // global semaphore visible to all processes
int *shared_variable;


void *create_shared_memory_area()
{
        void *addr;

        addr = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED)
        {
                perror("mmap");
                exit(1);
        }
        return addr; 
}

void destroy_shared_memory_area(void *addr)
{

        if (munmap(addr, sysconf(_SC_PAGE_SIZE)) == -1)
        {
                perror("destroy_shared_memory_area: munmap failed");
                exit(1);
        }
}

void sigint_handler(int sig_number){
        printf("Signal SIGINT found! Total number of processes searching the file: %d\n", num_processes);
        printf("Signal SIGINT found! Total number of characters found untill now are %d\n", *shared_variable);
}

void end_of_child(){
        num_processes--;
}


void char_counter(int fdr, char c_check, off_t start_pos, off_t end_pos){
    char c2c = c_check, cc;
    ssize_t rcnt;
    off_t bytes_read = 0;

    // Ensure the start position is set once per process
    if(lseek(fdr, start_pos, SEEK_SET) == -1){
        perror("lseek");
        exit(1);
    }

    while(bytes_read < (end_pos - start_pos) && (rcnt = read(fdr, &cc, 1)) > 0) {
        if (rcnt == -1) {
            perror("read");
            exit(1);
        }
	usleep(10);
        if(cc == c2c) {
            if(sem_wait(&semaphore) != 0) {
                perror("sem_wait");
                exit(1);
            }
	    usleep(100000); // sleep for 0,1 seconds
            (*shared_variable)++;
           // printf("Process number %d prints the total count for character '%c' so far: %d time(s)\n", getpid(), cc, *shared_variable);
            if(sem_post(&semaphore) != 0) {
                perror("sem_post");
                exit(1);
            }
        }
        bytes_read++;
    }
}

int main(int argc, char **argv){

        if(argc != 3){
                perror("User gave us the wrong call arguments.\n");
                exit(1);
        }

        shared_variable = create_shared_memory_area();
        *shared_variable = 0;
         // allocating memory for shared semaphores

        if (sem_init(&semaphore, 1, 1) < 0)
        // initialize the semaphore, shared as for the first '1' and enabled as for the second '1'
        {
                perror("sem_init");
                exit(1);
        }

        signal(SIGINT, &sigint_handler);
        signal(SIGUSR1, end_of_child);

        pid_t p;

        int status;

        off_t end_pos; //end of reading for each child process
        off_t start_pos = 0; //initial starting position for the first child
        off_t file_size, segment_size;

        /* opening the file to get its size */
        int fd = open(argv[1], O_RDONLY);
        if(fd == -1){
                perror("open");
                exit(1);
        }
        file_size = lseek(fd, 0, SEEK_END);

        /* calculating the segment size for each child process */
        segment_size = file_size / P;

        /* if the file size is not evenly divisible we adjust the segment for the last process */

        if(file_size % P != 0){ segment_size++; }

        for(int i = 0; i < P; i++){

                num_processes++;
                p = fork();

                if(p == -1){
                        perror("fork");
                        exit(1);
                }
                else if(p == 0){
                        signal(SIGINT, SIG_IGN);

			int fd_local = open(argv[1], O_RDONLY);
                        end_pos = start_pos + segment_size; //the end position for the current child

                        if (i == P-1)end_pos = file_size;
                        printf("Child process %d searching the file ...\n", getpid());
                        printf("Child process %d starting from position %ld to %ld\n", getpid(), start_pos, end_pos);

                        sleep(1);
                        char_counter(fd_local, argv[2][0], start_pos, end_pos);

//                      sleep(2); //for 2 seconds pause, testing SIGINT
                        kill(getppid(), SIGUSR1);
                        sleep(1);

                        destroy_shared_memory_area(shared_variable);

                        exit(0);
                }
                start_pos += segment_size; /*parent process updates the starting
                                                 position for the next child*/
        }
        //parent now
        int terminated_processes = 0;

        while (terminated_processes < P) {
                wait(&status); // Decrement for each child process that finishes
                terminated_processes ++;
        }
        int counter = *shared_variable;
        printf("The total number of times character %c occurred: %d\n", argv[2][0], counter);
       // printf("The total number of processes was %d\n", P);

        if (sem_destroy(&semaphore) < 0){
                perror("sem_destroy");
                exit(1);
        }
        if (-1 == close(fd))
                perror("close");

        destroy_shared_memory_area(shared_variable); //munmap
        exit(0);
}
