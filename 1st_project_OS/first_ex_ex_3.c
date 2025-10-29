#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
//#include <time.h>

#define P 5 //number of child processes

int num_processes = 0;
int total_count = 0;

void sigint_handler(int sig_number){
        printf("Signal SIGINT found! Total number of processes searching the file: %d\n", num_processes);
//      printf("The total number of times character %c occurred: %d\n", argv[2][0], total_count);
        exit(0);
}

int char_counter(char *file, char c_check, off_t start_pos, off_t end_pos){ //for character count
        char c2c = 'a', cc;
        int count = 0;

        int fdr = open(file, O_RDONLY);
        if(fdr == -1){
                perror("open");
                exit(1);
        }

        if(lseek(fdr, start_pos, SEEK_SET) == -1){
                perror("lseek");
                exit(1);
        }

        ssize_t rcnt;
        c2c = c_check;

        while(rcnt = read(fdr, &cc, sizeof(cc)) > 0){
                if(rcnt == -1){
                        perror("read");
                        exit(1);
                }
                if(cc == c2c) count++;
                if(lseek(fdr, 0, SEEK_CUR) >= end_pos) break; //stop reading if reached the end position
        }
        close(fdr);
        return count;
}

int main(int argc, char **argv){

        if(argc != 3){
                perror("User gave us the wrong call arguments.\n");
                exit(1);
        }

        signal(SIGINT, sigint_handler);

        int pipesfd[P][2]; //array from pipelines for each child process
        for(int i = 0; i < P; i++){
                if(pipe(pipesfd[i]) == -1){
                        perror("pipe");
                        exit(1);
                }
        }

        pid_t p;

        int status;

        off_t end_pos; //end of reading for each child process
        off_t start_pos = 0; //initial starting position for the first child
        off_t file_size, segment_size;

        /* If we want to count the time of each process */
//      clock_t start_time[P];
//      clock_t end_time[P];

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

                p = fork();

                if(p == -1){
                        perror("fork");
                        exit(1);
                }

                else if(p == 0){
                        end_pos = start_pos + segment_size; //the end position for the current child

                        printf("Child process %d searching the file ...\n", getpid());
                        printf("Child process %d starting from position %ld to %ld\n", getpid(), start_pos, end_pos);

//                      start_time[i] = clock(); //start the clock

                        int count = char_counter(argv[1], argv[2][0], start_pos, end_pos);

                        sleep(2); //for testing SIGINT

                        printf("Child process %d found character %c %d time(s)\n", getpid(), argv[2][0], count);

                        close(pipesfd[i][0]);
                        if(write(pipesfd[i][1], &count, sizeof(count)) == -1){
                                perror("write");
                                exit(1);
                        }
                        close(pipesfd[i][1]);

//                      end_time[i] = clock();

                        exit(0);

                }
                start_pos += segment_size; /*parent process updates the starting
                                                 position for the next child*/
        }
        //parent now
        int received_char_count;

        while(wait(&status) > 0){ num_processes++; }

        for(int i = 0; i < P; i++){
                close(pipesfd[i][1]);
                if(read(pipesfd[i][0], &received_char_count, sizeof(received_char_count)) == -1){
                        perror("read");
                        exit(1);
                }
                total_count += received_char_count;

                close(pipesfd[i][0]);
//              printf("Child process %d ran for %f seconds.\n", i, (double)(end_time[i] - start_time[i]) / CLOCKS_PER_SEC);
        }

        printf("The total number of times character %c occurred: %d\n", argv[2][0], total_count);

        exit(0);
}














