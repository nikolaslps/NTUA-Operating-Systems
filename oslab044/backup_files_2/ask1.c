#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main (int argc, char **argv){

	int fdr, fdw;
	char c2c = 'a', cc;
	int count = 0;

	fdr = open(argv[1], O_RDONLY)
	if (fdr == -1){
	perror("open");
	exit(1);
	}

	fdw = open(argv[2], O_WRONLY)
	if (fdw == -1){
	perror("open");
	exit(1);
	}

	while (rcnt = read(fdr, cc, 1) > 0)
		if (cc == c2c) count++;

	if (rcnt == -1){
		perror("read");
		exit(1);
		}
	 char output[50];
   	 int length = snprintf(output, 50, "Character '%c' found %d times\n", c2c, count);

    	wcnt = write(fdw, output, length);
	if (wcnt == -1) {
	perror("write");
	exit(1);
	}

	close(fdr);
	clode(fdw);

	return 0;
}
