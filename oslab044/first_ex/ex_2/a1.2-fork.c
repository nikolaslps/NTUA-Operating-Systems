#include <stdio.h>
#include <sys/types.h> //for pid_t()
#include <unistd.h> //for fork() and getpid() 
#include <stdlib.h> //for exit()
#include <sys/wait.h> //for wait()

int main (){
	pid_t p, mypid;
	int x = 11;
	p = fork();
	int status; //argument for wait

	if (p < 0){
		perror("fork");
		exit(1);
	}
	else if (p == 0){
		x = 10;
		mypid = getpid();
		pid_t father_pid = getppid(); //gives the pid of the child's father
		printf("Hello world! My pid is %d and my father's pid is %d\n", mypid, father_pid); 
		printf("The value of x for me, the child, is: %d\n", x);
		exit(0);
	}
	else {
		wait(&status); //wait until child terminates
		x = 3;
		printf("My child's pid is %d\n", p);
		printf("The value of x for father is: %d\n", x);
		exit(0);
	}
}
