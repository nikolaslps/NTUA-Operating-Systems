#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#define most_workers 20

volatile sig_atomic_t responseReady = 0;

int pipefdwrite[2];
int pipefdread[2];

void signalHandler (int sig){
    if (sig == SIGUSR2)
        responseReady = 1;
    printf("\nsignal received back\n");
}

void callend(){
    wait(NULL);
    int front_end_count;
        if (read(pipefdread[0], &front_end_count, sizeof(front_end_count)) == -1)
    {
        perror("read");
        exit(1);
    }

    printf("Total_count is %d\n", front_end_count);
    exit(0);
}

void signalHandler2 (int sig){
    printf("\nThe dispatcher has ended\n");

    callend();

}


int main (int argc, char **argv){


    if (argc != 3){
        perror("wrong argumenrs");
        exit(1);
    }

    signal(SIGUSR1, signalHandler2);
    signal(SIGUSR2, signalHandler);

     if (pipe(pipefdwrite) == -1 || pipe(pipefdread) == -1)
    {
        perror("pipe");
        exit(1);
    }

    pid_t dispatcher;

    dispatcher = fork();

    if (dispatcher < 0){
        perror("fork");
        exit(1);
    }

    else if (dispatcher == 0){

        close (pipefdwrite[1]);
        close (pipefdread[0]);

        dup2(pipefdwrite[0], STDIN_FILENO);
        close(pipefdwrite[0]);

        dup2(pipefdread[1], STDOUT_FILENO);
        close(pipefdread[1]);

        char *argv2[] = {"./dispatcher", argv[1], argv[2], NULL};
        execv(argv2[0], argv2);//executing ./dispatcher (original code) with the provided arguments
        perror("execv");
        exit(1);
    }

    else {//This is the front_end part now

          close(pipefdwrite[0]);  // Close the read-end of the pipe

      while (1) {
            printf("Enter a command \n");
            printf("\nThe accessible commands are: \n");
            printf("1) 'increase x (enter)': adding x workers\n");
            printf("2) 'decrease x (enter)': killing x workers\n");
            printf("3) 'terminate (enter)': killing the first active worker\n");
            printf("4) 'progress (enter)': for file percentage and count of character till now\n");
            printf("5) 'info (enter)': for info\n");
            printf("6) 'exit (enter)': to proceed\n");
            char input[100];
            int status;
            pid_t result = waitpid(dispatcher, &status, WNOHANG);
            if (result != 0) {
                break;
            }

            if (fgets(input, sizeof(input), stdin) == NULL || strncmp(input, "exit", 4) == 0) {
                break;  // Exit loop if user types 'exit' or on error
            }

            // Remove newline
            input[strcspn(input, "\n")] = 0;
            printf("sending signal %s", input);

            if (write(pipefdwrite[1], input, strlen(input) + 1) == -1) {
                perror("write");
                break;
             }

            // Notify the dispatcher that there's new data
            kill(dispatcher, SIGUSR1);

        while (!responseReady) {
        sleep(1);
    }
        responseReady = 0;
        char response[1024];

      ssize_t nbytes = read(pipefdread[0], response, sizeof(response) - 1); // Leave space for null ter>            if (nbytes == -1) {
             perror("read");
                exit(1);
            }
        else {
    response[nbytes] = '\0'; // Null-terminate the string
}
        printf("%s\n", response);
        printf("\n");

        }

        printf("User chose to exit. Waiting for the dispatcher to end\n");

        close(pipefdwrite[1]);  // Close the write-end of the pipe
        wait(NULL);  // Wait for the dispatcher to ensure it has finished
    }

    }

