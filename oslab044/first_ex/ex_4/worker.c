#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>


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
        perror("lseek from worker");
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


int main (int argc, char **argv){

    
int counter2 = 0;
char cc = argv[2][0];
off_t start = atoll(argv[3]);
off_t end = atoll(argv[4]);
int counter = char_counter(argv[1], cc, start, end);
if (argc > 5){
    off_t start_remaining = atoll(argv[5]);
    off_t end_remaining = atoll(argv[6]);
    counter2 = char_counter(argv[1], cc, start_remaining, end_remaining);
}
//printf("%d", counter);
counter += counter2;
    sleep(3);
    
 if (write(STDOUT_FILENO, &counter, sizeof(counter)) == -1)
    {
        perror("write");
        exit(1);
    }
	sleep(2);
exit(0);
}
