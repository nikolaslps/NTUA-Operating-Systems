#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define most_workers 10

// global scope

off_t segment_size, file_size;
off_t reached_end = 0;
off_t start, end;             // Start and end of the segment of the file that each child procesess
double file_percentage;       // Percentage of file workers have worked on
int count = 0;                // This is the total count that the dispatch will receive
int pipefd2[most_workers][2]; // Pipeline for worker-dispatch communication

typedef struct // This struct contains information about all workers
{              // Typedef because we will store this information in an array
    pid_t pid;
    off_t start;
    off_t end;
    int active;
} Task;

Task WORKERS[most_workers]; // This is an array that stores the workers

off_t save_start[most_workers]; // This will save all the starts if terminated active workers
off_t save_end[most_workers];   // Same for end

int save_the_search_of_dead_child = 0; // This is useful for worker who is terminated while working
int total_current_workers = 5;         // This includes all current workers who have either started or not
int remaining_workers = 5;
int terminated_children = 0;

// Here we will define all the functions we will use
int check_remaining_file_search();
void handle_sigchild(int sig); // int sig stores the input signal tha triggered this function
void handle_increase(int sig);
void handle_decrease(int sig);
void handle_kill_active_worker(int sig);
int char_counter(char *file, char c_check, off_t start_pos, off_t end_pos);
void setup_signal_handling();

int check_remaining_file_search()
{ // This will check if a process was terminated while searching and has left it's share of work still unchecked
    for (int j = 0; j < most_workers; j++)
    {
        if (save_start[j] != -1) // I only check start as end will always have the same values as start here
            return j;
    }
    return -1;
}

void handle_sigchild(int sig) // This signal informs the dispatcher that a child finished it's work correctly
{
    int status;
    pid_t pid;
    int counter, i;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("\nEntering void handle_sigchild(int sig)\n");
        printf("Child pid %d, child status %d\n", pid, status);
        if (WIFEXITED(status))
        {
            // Find which worker has exited to read the correct pipe
            for (i = 0; i < most_workers; i++)
            {
                if (WORKERS[i].pid == pid)
                { // Assume you have a workers array tracking PIDs
                    printf("Worker with pid = %d found to set inactive after finishing his task\n", WORKERS[i].pid);

                    close(pipefd2[i][1]);
                    if (read(pipefd2[i][0], &counter, sizeof(counter)) == -1)
                    {
                        perror("write");
                        exit(1);
                    }
                    count += counter;
                    printf("\nCounter = %d\n", counter);
                    printf("Count = %d\n", count);

                    close(pipefd2[i][0]); // Close the read end after reading

                    WORKERS[i].active = 0;
                    WORKERS[i].start = -1;
                    WORKERS[i].end = -1;

                    // WORKERS[i].pid = -1;
                    terminated_children++;
                    break; // Mark the worker as inactive
                }
            }
        }
    }
    if (WORKERS[i].end > reached_end)
    {
        reached_end = WORKERS[i].end;
    }

    double file_percentage; // To find the percentage of searched_file
    off_t current_file_searched = end;

    int A[most_workers];
    for (int i = 0; i < most_workers; i++)
    {
        A[i] = check_remaining_file_search();
        if (A[i] != -1)
            current_file_searched -= save_end[i] - save_start[i]; // This subtracts the uncecked parts of the file
    }

    printf("\nThe end is: %d\n", end);
    printf("The current_file_searched is: %d\n", current_file_searched);

    file_percentage = ((double)current_file_searched / file_size) * 100.0;                  // Finding the percentage of checked file
    printf("\nThe percentage of the file that has been covered is: %f\n", file_percentage); // Printing the percentage everytime a worker finishes
    printf("\n-------------------------------- End of worker -------------------------------------\n");

    return;
}

void handle_increase(int sig) // Every time we increase we change the segment_size to divide the work in fair shares
{                             // increase workers
    int num_to_increase;
    scanf("%d", &num_to_increase);
    sleep(2);
    printf("Num_to_increase = %d\n",num_to_increase);

    if (total_current_workers < most_workers)
    {   
        int jobs_available = most_workers - total_current_workers;
        if(num_to_increase > jobs_available)
        { 
            printf("Not so many jobs available\n"); 
        }
        else 
        {
            printf("Just hired %d worker(s)! Time for work.\n");

            total_current_workers += num_to_increase;
            remaining_workers += num_to_increase;
            segment_size = (file_size - end) / remaining_workers;
        }
    }
    else
        exit(1);
}

void handle_decrease(int sig) // Exactly same as in increase
{                             // decrease workers
    int num_to_decrease;
    scanf("%d", &num_to_decrease);
    sleep(2);
    printf("Num_to_decrease = %d\n",num_to_decrease);

    if (remaining_workers > 1)
    {
        if(num_to_decrease > remaining_workers)
        {
            printf("We can't afford so many layoffs\n");
        }
        else
        {
            printf("No money ... We fired %d workers\n", num_to_decrease);

            total_current_workers -= num_to_decrease;
            remaining_workers -= num_to_decrease;
            segment_size = (file_size - end) / remaining_workers;
        }
    }
    else
        exit(1);
}

void handle_kill_active_worker(int sig)
{ // This will kill the first active worker it finds
    for (int i = 0; i < most_workers; i++)
    {
        if (WORKERS[i].active == 1)
        { // If active worker found, we kill him and save its data
            kill(WORKERS[i].pid, SIGKILL);

            save_start[save_the_search_of_dead_child] = WORKERS[i].start; // Stores the start of the file
            save_end[save_the_search_of_dead_child] = WORKERS[i].end;     // Stores the end of the file
            save_the_search_of_dead_child++;

            // We set the variables of the deactivated worker to 'zero'
            WORKERS[i].pid = -1;
            WORKERS[i].start = -1;
            WORKERS[i].end = -1;
            WORKERS[i].active = 0;
            break;
        }
    }
}

int char_counter(char *file, char c_check, off_t start_pos, off_t end_pos)
{ // for character count
    char c2c = 'a', cc;
    int count = 0;

    int fdr = open(file, O_RDONLY);
    if (fdr == -1)
    {
        perror("open");
        exit(1);
    }

    if (lseek(fdr, start_pos, SEEK_SET) == -1)
    {
        perror("lseek");
        exit(1);
    }

    ssize_t rcnt;
    c2c = c_check;

    while ((rcnt = read(fdr, &cc, sizeof(cc))) > 0)
    {
        if (cc == c2c)
            count++;
        if (lseek(fdr, 0, SEEK_CUR) >= end_pos)
            break; // stop reading if reached the end position
    }

    if (rcnt == -1)
    {
        perror("read");
        exit(1);
    }
    close(fdr);
    return count;
}

void setup_signal_handling()
{
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

    // Setting up SIGINT handler
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_increase;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL); // activated with 'Ctrl+C'

    // Setting up SIGTSTP handler
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_decrease;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL); // activated with 'Ctrl+Z'

    // Setting up SIGQUIT handler
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_kill_active_worker;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGQUIT, &sa, NULL); // activated with 'Ctrl+\'
}

int main(int argc, char **argv)
{
    int status1; // wait for dispatcher to finish

    printf("Here we are before the setup of all workers to inactive\n");

    for (int j = 0; j < most_workers; j++) // deaming all the wrokers innactive
    {
        save_start[j] = -1;
        save_end[j] = -1;
        WORKERS[j].pid = -1;
        WORKERS[j].start = -1;
        WORKERS[j].end = -1;
        WORKERS[j].active = 0;
    }
    printf("Hello! We have already %d workers ready to count\n", total_current_workers);
    printf("We have enabled the following commands for you: \n");
    printf("For increasing the workers press 'Ctrl+C'\n");
    printf("For decreasing the workers press 'Ctrl+Z'\n");
    printf("For killing an active worker press 'Ctrl+\'\n");
    sleep(2);

    printf("Here we are after the setup of all workers to inactive\n");

    int pipefd[2]; // communication from front_end to dispatcher
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(1);
    }

    // first fork() to make the dispatcher
    pid_t p = fork();

    if (p < 0)
    {
        perror("fork");
        exit(1);
    }

    else if (p == 0) // we are now entering the dispatcher
    {
        printf("\nHere we are entering the front_end's child, the dispatcher.\n");

        int status2; // wait till all workers have finished their process

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
        printf("\nFile size: %d, segment_size: %d\n", file_size, segment_size);

        int current_worker = 0;

        end = start + segment_size;

        setup_signal_handling();

        while (start < file_size)
        {
            printf("\nEntering the while loop\n");

            int check_remaining = check_remaining_file_search();

            printf("Remaining check %d\n", check_remaining);

            if (check_remaining != -1)
            {
                off_t save_start_of_dead_child = save_start[check_remaining];
                off_t save_end_of_dead_child = save_end[check_remaining];

                save_start[check_remaining] = -1;
                save_end[check_remaining] = -1;
            }

            if (end > file_size)
                end = file_size;

            if (current_worker == total_current_workers - 1)
                end = file_size;

            printf("Start variable = %d\n", start);
            printf("End variable = %d\n", end);
            printf("\n");

            pid_t for_work; // This will fork for workers
            if (pipe(pipefd2[current_worker]) == -1)
            {
                perror("pipe");
                exit(1);
            }

            for_work = fork();

            WORKERS[current_worker].pid = for_work;
            WORKERS[current_worker].start = start;
            WORKERS[current_worker].end = end;
            WORKERS[current_worker].active = 1;

            if (for_work < 0)
            {
                perror("fork");
                exit(1);
            }

            else if (for_work == 0) // Now we are entering the worker
            {
                printf("Printing the worker %d\n", current_worker);
                pid_t worker_pid = getpid();
                printf("Here we are entering the dispatcher's child, the worker with pid %d\n", worker_pid);
                printf("\n-------------------------------- Start of worker -------------------------------------\n");

                int previous_counter = 0;
                if (check_remaining != -1)
                {
                    previous_counter = char_counter(argv[1], argv[2][0], save_start[check_remaining], save_end[check_remaining]);

                    printf("Printing the previous counter inside the if (check_remaining != -1) %d\n", previous_counter);
                }
                int child_counter = char_counter(argv[1], argv[2][0], start, end);

                printf("\nPrinting the counter %d\n", child_counter);

                child_counter += previous_counter;

                printf("Printing the previous counter outside the if (check_remaining != -1) %d\n", previous_counter);

                printf("Printing the child's counter %d\n", child_counter);

                //...This is were the pipe will send the information from workers to the dispatcher
                close(pipefd2[current_worker][0]);

                // printf("Printing the current worker %d\n", current_worker); // begins counting from 0

                if (write(pipefd2[current_worker][1], &child_counter, sizeof(child_counter)) == -1)
                {
                    perror("write");
                    exit(1);
                }

                close(pipefd2[current_worker][1]);

                // handle_sigchild(WEXITSTATUS(worker_pid));

                exit(0); // If it finishes correctly
            }            // This is where the worker finishes

            sleep(2);
            start = end;
            end += segment_size;
            remaining_workers--;
            current_worker++;

            printf("Printing the remaining workers %d\n", remaining_workers); // in the last iteration of while we take remaining_workers = -1
            printf("\n");

            sleep(2);
        } // This is where the while loop ends

        // int terminated_children = 0;
        pid_t child_pid;
        int child_status;

        printf("\nTotal count outside while loop = %d\n", count);

        /* This while loop might not be necessary because when a worker dies
        we already set terminated_children++ so we won't enter this loop anyways.
        If we delete terminated_children++ from the sig_child function then the while
        loop is necessary indeed */
        while (terminated_children < total_current_workers)
        {
            printf("\nHere we are entering the while (terminated_children < total_current_workers){}.\n");
            printf("\nPrinting the number of terminated_children %d\n", terminated_children);
            printf("\nTotal count inside while loop = %d\n", count);

            child_pid = waitpid(-1, &child_status, 0); // Wait for any child process

            printf("Printing the child_pid %d\n", child_pid);

            if (child_pid > 0) // it is greater than zero if the child has terminated correctly
            {
                printf("Found a child pid > 0\n");

                terminated_children++;
                // Optional: Process the termination status of the child
                if (WIFEXITED(child_status))
                {
                    printf("Child %d terminated with exit status %d\n", child_pid, WEXITSTATUS(child_status));
                }
                else if (WIFSIGNALED(child_status))
                {
                    printf("Child %d terminated by signal %d\n", child_pid, WTERMSIG(child_status));
                }
            }
            else if (child_pid == -1)
            {
                printf("Found a child pid == -1\n");

                perror("waitpid");
                // Handle errors here (e.g., all children are already terminated)
                // exit(1);
                break;
            }
            printf("\nHere we exit the while (terminated_children < total_current_workers){} correctly.\n");
        }

        int getting_the_total_count = count;
        printf("\nThe getting_the_total_count variable = %d\n", getting_the_total_count);

        // Here is something unecessary inside the dispatcher .........
        printf("\nLet's check something inside the dispatcher ... \n");
        printf("The workers' characteristics are:\n");
        for (int i = 0; i < total_current_workers; i++)
        {
            printf("\npid = %d,\nactive = %d,\nstart = %d,\nend = %d\n", WORKERS[i].pid, WORKERS[i].active, WORKERS[i].start, WORKERS[i].end);
        }
        // The unecessary code ends here ..........

        //...This is were the pipe will send the information from dispatcher to the front end
        
        //close(pipefd[0]); //WHY THIS CAUSES ERROR??????

        if (write(pipefd[1], &count, sizeof(count)) == -1)
        {
            perror("write");
            exit(1);
        }
        close(pipefd[1]);

        printf("\nThe count variable = %d\n", count);
        
        /* When we press Ctrl C or Ctrl Z the code terminates here. WHY? And if
        close(pipefd[0]) is non-comment then it terminates after printing the
        workers' characteristics. WHY? */

        exit(0);

    } // This is where the dispathcher ends
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    wait(&status1);

    printf("\nReatched the front end now!\n");

    // here is where the front end collects the info from dispatcher
    int front_end_count;

    printf("\nThe front_end_count = %d\n", front_end_count);

    // Here is something unecessary inside the front end .........
    printf("\nLet's check something inside the front end ... \n");
    printf("The workers' characteristics are:\n");
    for (int i = 0; i < total_current_workers; i++)
    {
        printf("\npid = %d,\nactive = %d,\nstart = %d,\nend = %d\n", WORKERS[i].pid, WORKERS[i].active, WORKERS[i].start, WORKERS[i].end);
    }
    // The unecessary code ends here ..........

    close(pipefd[1]);
    if (read(pipefd[0], &front_end_count, sizeof(front_end_count)) == -1)
    {
        perror("read ... problem");
        exit(1);
    }
    close(pipefd[0]);

    printf("The total characters (front_end_count) we found that machare %d\n", front_end_count);
    exit(0);

}
