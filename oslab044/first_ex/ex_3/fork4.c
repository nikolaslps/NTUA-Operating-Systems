#include <stdio.h>
#include <sys/types.h> //for pid_t()
#include <unistd.h> //for fork() and getpid() 
#include <stdlib.h> //for exit()
#include <sys/wait.h> //for wait()
#include <fcntl.h> //for O_RDONLY

int counter (char *file, char c_check){
	char c2c = 'a', cc;
	int count = 0;
	int fdr;

	fdr = open(file, O_RDONLY);
	if (fdr == -1){
		perror("open");
		exit(1);
	}

	ssize_t rcnt;
	c2c = c_check;

	while (rcnt = read(fdr, &cc, 1) > 0){
		if (rcnt == -1){
			perror("read");
			exit(1);
		}
		if (cc == c2c) count++;
	}
	return count;
}

int main (int argc, char **argv){

	if (argc != 3){
		perror("The user gave us the wrong calls. More arguments needed!\n");
		exit(1);
	}

	int fdr, fdw;
	int count;

        pid_t p, mypid;

        int x = 11;

	int pipefd[2]; /* pipefd[0] read end of the pipe
			pipefd[1] write end of the pipe */

	if(pipe(pipefd) == -1){ //creating the pipe (must be created before fork())
		perror("pipe");
		exit(1);
	}

        p = fork();
        int status; //argument for wait

        if (p < 0){
                perror("fork");
                exit(1);
        }

        else if (p == 0){ /*Child's time*/

                x = 10;

                mypid = getpid();
                pid_t father_pid = getppid(); //gives the pid of the child's father
                printf("Hello world! My pid is %d and my father's pid is %d\n", mypid, father_pid); 

                printf("The value of x for me, the child, is: %d\n", x);

		count = counter(argv[1],argv[2][0]);
		printf("The given character %c is found %d times\n", argv[2][0], count);

		close(pipefd[0]); //close the read end of pipe

		if(write(pipefd[1], &count, sizeof(count)) == -1){ //write the count into the pipe
			perror("write");
			exit(1);
		}

		close(pipefd[1]); //close the write end of the pipe in the child

		exit(0);
        }
        else {
                wait(&status); //wait until child terminates

                x = 3;

                printf("My child's pid is %d\n", p);
                printf("The value of x for father is: %d\n", x);

		close(pipefd[1]); //close the write end of the pipe in the parent

		int received_count; 
		if(read(pipefd[0], &received_count, sizeof(received_count)) == -1){
			perror("read");
			exit(1);
		}
		printf("The given character %c is found %d times by my child\n", argv[2][0],received_count);

		close(pipefd[0]); //close the read end of the pipe in the parent 

		exit(0);
        }
}
