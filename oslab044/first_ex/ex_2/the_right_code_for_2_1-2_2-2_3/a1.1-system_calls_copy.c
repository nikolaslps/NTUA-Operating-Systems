#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv){

	if (argc != 4){
		perror("the user gave us the wrong calls");
		exit(1);
	}
        int fdr, fdw;
        char c2c = 'a', cc;
        int count = 0;
        ssize_t rcnt, wcnt;

        fdr = open(argv[1], O_RDONLY);
                if (fdr == -1){
                perror("open");
                exit(1);
        }

        fdw = open(argv[2], O_WRONLY | O_CREAT, 0644);
                if (fdw == -1){
                perror("open");
                exit(1);
        }

        c2c = argv[3][0];

        while (rcnt = read(fdr, &cc, 1) > 0){
                if (rcnt == -1){
                perror("read");
                exit(1);}

                 if (cc == c2c) count++;
        }

        char output[50];
        int length = snprintf(output, 50, "The character '%c' appears %d times.\n", c2c, count);

        wcnt = write(fdw, output, length);
                if (wcnt == -1) {
                perror("write");
                exit(1);
        }

        close(fdr);
        close(fdw);

        return 0;
}
