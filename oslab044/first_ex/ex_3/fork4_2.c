#include <stdio.h>
#include <sys/types.h> //for pid_t()
#include <unistd.h> //for fork() and getpid() 
#include <stdlib.h> //for exit()
#include <sys/wait.h> //for wait()
#include <fcntl.h> //for O_RDONLY


int main(int argc, char *argv[]){

	if(argc != 2){
		perror("Wrong calls were given");
		exit(1);
	}

	char cc = argv[1][0];
	int status;

	pid_t p = fork();

	if (p == -1){
		perror("fork");
		exit(1);
	}

	else if (p == 0){
		char *argv2[] = {"./a1.1-system_calls_copy", "my_name.txt", "output.txt", &cc, NULL};

		execv(argv2[0], argv2);
		perror("execv");
		exit(1);
	}

	else{
		wait(&status);
	}

	return 0;
}
