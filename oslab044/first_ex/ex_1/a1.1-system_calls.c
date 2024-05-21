#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv){

        int fdr, fdw;//file descriptors
        char c2c = 'a', cc;
        int count = 0;
	ssize_t rcnt, wcnt;//For read and write

        fdr = open(argv[1], O_RDONLY);//We open the file and assign value fdr
        	if (fdr == -1){
        	perror("open");//If the file doesn't open we receivw error
        	exit(1);//we exit with an error
        }

        fdw = open(argv[2], O_WRONLY | O_CREAT, 0644);//Same process as before
        	if (fdw == -1){
        	perror("open");
        	exit(1);
        }

	c2c = argv[3][0];//We assign the search character to c2c

	while ((rcnt = read(fdr, &cc, 1)) > 0)//While read check
		 if (cc == c2c) count++;

		if (rcnt == -1){//If rcnt == -1 we encountered a fatal error
                perror("read");
                exit(1);}

        char output[50];
        int length = snprintf(output, 50, "The character '%c' appears %d times\n", c2c, count);
//We store the above on array output as well as the number of characters which is returned from snprintf to length

        wcnt = write(fdw, output, length);//We write the ouptut array to the output file
        	if (wcnt == -1) {
        	perror("write");
        	exit(1);
        }

        close(fdr);//closing files
        close(fdw);

        return 0;
}

