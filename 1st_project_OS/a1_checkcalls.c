#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char **argv){

        if (argc != 4){//Checks if user gave exactly three arguments
        perror("The user gave the wrong amount of calls");
        exit(1);
        }

        if (strlen(argv[3]) != 1){//Checks if third argument is a character
        perror("not character");
        exit(1);
        }

        if (access(argv[1], R_OK) == -1) {//Checks if read is available from first file
        perror("access");
        exit(1);
        }

        if (access(argv[2], F_OK) != -1 && access(argv[2], W_OK) == -1) {
        perror("access");
        exit(1);// This part ends the program if the write file exists but is not readable
    }

        int fdr, fdw;//file descriptors
        char c2c = 'a', cc;
        int count = 0;
        ssize_t rcnt, wcnt;//For read and write

        fdr = open(argv[1], O_RDONLY);//We open the file and assign value fdr
                if (fdr == -1){
                perror("open of first file");//If the file doesn't open we receivw error
                exit(1);//we exit with an error
        }

        fdw = open(argv[2], O_WRONLY | O_CREAT, 0644);
                if (fdw == -1){//With O_CREAT if ths WR file doesn't exist it is created
                perror("open of second file");
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
//We store the above on array output as well as the number of characters which is returned from snprint>
        wcnt = write(fdw, output, length);//We write the ouptut array to the output file
                if (wcnt == -1) {
                perror("write");
                exit(1);
        }

        close(fdr);//closing files
        close(fdw);

        return 0;
}
