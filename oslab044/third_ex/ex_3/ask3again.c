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

        addr = mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_S>
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
        printf("Signal SIGINT found! Total number of processes searching the fi>
        printf("Signal SIGINT found! Total number of characters found untill no>
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

    while(bytes_read < (end_pos - start_pos) && (rcnt = read(fdr, &cc, 1)) > 0)>
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
           // printf("Process number %d prints the total count for character '%>
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

    if (sem_init(&semaphore, 1, 1) < 0){
        perror("sem_init");
        exit(1);
    }

    signal(SIGINT, &sigint_handler);
    signal(SIGUSR1, end_of_child);

    // Open file once in the parent
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(1);
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    off_t segment_size = file_size / P;
    if(file_size % P != 0){ segment_size++; }

    off_t start_pos = 0; // Initial starting position for the first child
    for(int i = 0; i < P; i++){
        num_processes++;
        pid_t p = fork();
        if(p == -1){
            perror("fork");
            exit(1);
        }
        else if(p == 0){
            signal(SIGINT, SIG_IGN);
            off_t end_pos = start_pos + segment_size;
            if (i == P-1) end_pos = file_size;
            printf("Child process %d searching the file ...\n", getpid());
            printf("Child process %d starting from position %ld to %ld...\n", getpid(), start_pos, end_pos);
            if(lseek(fd, start_pos, SEEK_SET) == -1){
                perror("lseek");
                exit(1);
            }
            char_counter(fd, argv[2][0], start_pos, end_pos);
            close(fd); // Close in child when done
            destroy_shared_memory_area(shared_variable);
            exit(0);
        }
        start_pos += segment_size; 
    }
    // Close file descriptor in parent after all children are forked
    close(fd);

    // Wait for all children to complete
    int terminated_processes = 0;
    int status;
    while (terminated_processes < P) {
        wait(&status);
        terminated_processes++;
    }
    printf("The total number of times character %c occurred: %d\n", argv[2][0], *shared_variable);
    if (sem_destroy(&semaphore) < 0){
        perror("sem_destroy");
        exit(1);
    }
    destroy_shared_memory_area(shared_variable);
    exit(0);
}
