#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define most_workers 20


   typedef struct // This struct contains information about all workers
{              // Typedef because we will store this information in an array
    pid_t pid;
    off_t workerstart;
    off_t workerend;
    int active;
} Task;

volatile sig_atomic_t signalReceived = 0;

char sending[100];

Task WORKERS[most_workers];

int current_worker = 0;
volatile sig_atomic_t command_available = 0;
volatile off_t segment_size; 
off_t file_size, start, end;
volatile int remaining_workers = 10;
volatile int total_current_workers = 10;
int terminated_children = 0;
int count = 0;

     int pipefdread[most_workers][2];


void sendMessageWithWrite(const char* message) {
    // Use write to send the message
    if (write(STDOUT_FILENO, message, strlen(message)) == -1) {
        perror("write");
        exit(1);
    }
}

void signalHandler(int sig){
    if (sig == SIGUSR1)
    command_available = 1;

}

void increase_workers(int value){
    char message[256];
    if (total_current_workers + value < most_workers){
        total_current_workers += value;
        remaining_workers += value;
        segment_size = (file_size - end)/remaining_workers;
        int length = snprintf(message, sizeof(message), "Workers increased by %d\n", value);
    if (length > 0 && length < sizeof(message)) {
        sendMessageWithWrite(message);
    } 
    else {
        perror("snprintf");
        exit(1);
    }
    }
    else {
         int length = snprintf(message, sizeof(message),"Couldn't increase because I cant have so many wokrers on my project\n");
    if (length > 0 && length < sizeof(message)) {
        sendMessageWithWrite(message);
    } 
    else {
        perror("snprintf");
        exit(1);
    }
}
}

void decrease_workers(int value){
    char message[256];
    if (remaining_workers - value >= 1){
        total_current_workers -= value;
        remaining_workers -= value;
        segment_size = (file_size - end)/remaining_workers;
        int length = snprintf(message, sizeof(message), "Workers decreased by %d\n", value);
    if (length > 0 && length < sizeof(message)) {
        sendMessageWithWrite(message);
    } 
    else {
        perror("snprintf");
        exit(1);
    }
    }
    else{
        int length = snprintf(message, sizeof(message),"Couldn't decrease because no one will be left on my project\n");
    if (length > 0 && length < sizeof(message)) {
        sendMessageWithWrite(message);
    } 
    else {
        perror("snprintf");
        exit(1);
    }
    } 
}

void get_info(){

    char message[256];
        int length = snprintf(message, sizeof(message), "All the available workers who have already completed their part or not are %d\n", total_current_workers);
    if (length > 0 && length < sizeof(message)) {
        sendMessageWithWrite(message);
    } 
    else {
        perror("snprintf");
        exit(1);
    }
    char message2[256];
    length = snprintf(message2, sizeof(message), "The remaining workers are %d\n", remaining_workers);
    if (length > 0 && length < sizeof(message)) {
        sendMessageWithWrite(message2);
    } 
        else {
        perror("snprintf");
        exit(1);
    }
    
}

void give_file_percentage_and_count_till_now(){
    double file_percentage;
    off_t examined = 0;
    int temp = current_worker;
        for (int i = 0; i <= temp; i++)
            if (WORKERS[i].active == 0 && WORKERS[i].workerend > examined)
            examined = WORKERS[i].workerend;

          

    file_percentage = (examined * 100) / file_size;

    
    char message[100]; 
    int message_length = snprintf(message, sizeof(message), "the percentage of the file examined up till now is %f\n", file_percentage);
    if (message_length < 0 || message_length >= sizeof(message)) {
        perror("snprintf");
        exit(1); 
    }


    ssize_t bytes_written = write(STDOUT_FILENO, message, message_length);
    if (bytes_written == -1) {
        perror("write");
        exit(1); // Exit or handle the error accordingly
    }

    // Prepare the message for total_count_till_now
    message_length = snprintf(message, sizeof(message), "The total_count_till now is %d\n", count);
    if (message_length < 0 || message_length >= sizeof(message)) {
        perror("snprintf");
        exit(1); // Handle or exit on error
    }

    // Write the message to STDOUT_FILENO
    bytes_written = write(STDOUT_FILENO, message, message_length);
    if (bytes_written == -1) {
        perror("write");
        exit(1); // Exit or handle the error accordingly
    }

    // /* this has to be delivered to front end using pipefdread write-end */
    // printf("the percentage of the file examined up till now is %d \n", file_percentage);
    // printf("The total_count_till now is %d\n", total_count_till_now);
}

void terminate_first_active_worker()
{

        if (remaining_workers < 1){
            char message[] = "You should have told me to kill the worker earlier\n";
             ssize_t bytes_written = write(STDOUT_FILENO, message, sizeof(message) - 1);
            if (bytes_written == -1) {
                perror("write");
                exit(1); // Exit or handle the error accordingly
            }
            return;
        }
    for (int i = 0; i <= current_worker; i++)
    {
        if (WORKERS[i].active == 1)
        { // If active worker found, we kill him and save its data

            kill(WORKERS[i].pid, SIGKILL);
            
            char message[] = "The first worker I found was killed\n";

            // Write the message to STDOUT_FILENO
            ssize_t bytes_written = write(STDOUT_FILENO, message, sizeof(message) - 1);
            if (bytes_written == -1) {
                perror("write");
                exit(1); // Exit or handle the error accordingly
            }


            // We set the variables of the deactivated worker to 'zero'
            WORKERS[i].pid = -1;
            WORKERS[i].active = -2;//The worker died by command and it's start and end must be restored
            break;
        }
    }
    total_current_workers--;
    terminated_children++;
}

int check_remaining_file_search()
{ // This will check if a process was terminated while searching and has left it's share of work still unchecked
    for (int j = 0; j <= current_worker; j++)
    {
        if (WORKERS[j].active == -2) // I only check start as end will always have the same values as start here
            return j;
    }
    return -1;
}

void processCommand(char *signalInfo) {
    char command[50];
    int value = 0;

    sscanf(signalInfo, "%s %d", command, &value);

    if (strcmp(command, "increase") == 0) {
        increase_workers(value);
    } else if (strcmp(command, "decrease") == 0) {
        decrease_workers(value);
    } else if (strcmp(command, "terminate") == 0) {
        terminate_first_active_worker();
    } else if (strcmp(command, "progress") == 0) {
        give_file_percentage_and_count_till_now();
    } else if (strcmp(command, "info") == 0) {
        get_info();
    }
}


void getcommand() {
    // Make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl SETFL");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int bytesRead = 0;
    while ((bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the string
        // Process the buffer content here. This simple example expects complete commands.
        processCommand(buffer);
    }
    if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    // Signal to the parent process that processing is done.
    kill(getppid(), SIGUSR2);
}


void handle_sigchild(int sig) // This signal informs the dispatcher that a child finished it's work correctly
{
    int status;
    pid_t pid;
    int counter, i;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))
        {
            // Find which worker has exited to read the correct pipe
    for (i = 0; i < most_workers; i++)
            {
    if (WORKERS[i].pid == pid)
{ // Assume you have a workers array tracking PIDs
    
    close(pipefdread[i][1]);
    if (read(pipefdread[i][0], &counter, sizeof(counter)) == -1)
    {
     perror("read");
    exit(1);
    }
    count += counter;

    close(pipefdread[i][0]); // Close the read end after reading

    WORKERS[i].active = 0;
    WORKERS[i].pid = -1;

    printf("%d\n", count);

                    // WORKERS[i].pid = -1;
    terminated_children++;
    break; // Mark the worker as inactive
                }
            }
        }
    }
}



int main (int argc, char **argv){


   signal(SIGUSR1, signalHandler);

    for (int j = 0; j < most_workers; j++) // deaming all the wrokers innactive
    {
        WORKERS[j].pid = -1;
        WORKERS[j].workerstart = -1;
        WORKERS[j].workerend = -1;
        WORKERS[j].active = 0;

    }

         int fdr = open(argv[1], O_RDONLY);
        if (fdr == -1)
        {
            perror("open");
            exit(1);
        }

         if (lseek(fdr, 0, SEEK_SET) == -1)
        {
            perror("lseek");
            exit(1);
        }

        file_size = lseek(fdr, 0, SEEK_END);
        segment_size = file_size / total_current_workers;
       

        int terminated_workers = 0;
        start = 0;
        end = start + segment_size;

    struct sigaction sa;

    // Setting up SIGCHLD handler
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigchild;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }


         while (start < file_size){

            if(command_available == 1){
                getcommand();
                command_available = 0;
            }

            if (end > file_size)
                end = file_size;

            if (current_worker == total_current_workers - 1)
                end = file_size;

                 pid_t worker;
                 
                 if (pipe(pipefdread[current_worker]) == -1){
                    perror("pipe");
                    exit(1);
                 }

            WORKERS[current_worker].active = 1;
            WORKERS[current_worker].workerstart = start;
            WORKERS[current_worker].workerend = end;

            off_t start_remaining = -1, end_remaining = -1;
            int a = check_remaining_file_search();
            if (a != -1){
                WORKERS[a].active = 0;
                start_remaining = WORKERS[a].workerstart;
                end_remaining = WORKERS[a].workerend;
            }
            

            worker = fork();

            WORKERS[current_worker].pid = worker;

            if (worker < 0){
                perror("fork");
                exit(1);
            }

            else if (worker == 0){
                    close (pipefdread[current_worker][0]);

                    dup2(pipefdread[current_worker][1], STDOUT_FILENO);
                    close(pipefdread[current_worker][1]);

                    char start_str[20]; // Ensure this buffer is large enough
                    char end_str[20];   // Ensure this buffer is large enough

                    snprintf(start_str, sizeof(start_str), "%lld", (long long)start);
                    snprintf(end_str, sizeof(end_str), "%lld", (long long)end);
                    char cc = argv[2][0];
                    if (a == -1){
                    char *argv2[] = {"./worker", argv[1], &cc, start_str, end_str, NULL};
                    execv("./worker", argv2);
                     exit(1);
                    }
                    
                    else {
                        char start_str2[20]; // Ensure this buffer is large enough
                        char end_str2[20];   // Ensure this buffer is large enough
                         snprintf(start_str2, sizeof(start_str2), "%lld", (long long)start_remaining);
                        snprintf(end_str2, sizeof(end_str), "%lld", (long long)end_remaining);
                        char *argv2[] = {"./worker", argv[1], &cc, start_str, end_str, start_str2, end_str2, NULL};
                        execv("./worker", argv2);
                     exit(1);
                    }

            }//end of worker body here
             
            start = end;
            end += segment_size;
            remaining_workers--;
            current_worker++;

            sleep(2);

             if(command_available == 1){
                getcommand();
                command_available = 0;
            }
		sleep(2);
         }//This is were the while loop ends
        
    while(terminated_children < total_current_workers)
        sleep(2);

        if (write(STDOUT_FILENO, &count, sizeof(count)) == -1)
    {
        perror("write");
        exit(1);
    }
    kill(getppid(), SIGUSR1);
}
