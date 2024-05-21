/*
 * a1.1-C.c
 *
 * A simple program in C counting the occurence of a character in a file
 * and writing the result in another file
 *
 * Input is given from the command line without further tests:
 * argv[1]: file to read from
 * argv[2]: file to write to
 * argv[3]: character to search for
 *
 * Operating Systems course, CSLab, ECE, NTUA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //to use the 'access()' function
#include <string.h> //to use strlen

int main(int argc, char *argv[]) {

    FILE *fpr, *fpw;
    char cc, c2c = 'a';
    int count = 0;
    
    //check if we have 3 input arguments (4 with the name of the executable file) 
    if(argc != 4){
	printf("We need: %s <file_to_read> <file_to_write> <char_to_search_for>\n", argv[0]);
	return -1;
    }

   /* if(access(argv[1], F_OK) == -1){
	printf("Error: %s is an invalid file to read from\n", argv[1]);
	return -1;
    }

    if(access(argv[2], F_OK) == -1){
	printf("Error: %s is an invalid file to write to\n", argv[2]);
	return -1;
    }

    if(strlen(argv[3]) != 1){
	printf("Error: The third argument must be a single character\n");
	return -1;
    }*/

    /* open file for reading */
    if ((fpr = fopen(argv[1], "r")) == NULL) {
        printf("Problem opening file to read\n");
        return -1; 
    }
    
    /* open file for writing the result */
    if ((fpw = fopen(argv[2], "w+")) == NULL) {
        printf("Problem opening file to write\n");
        return -1; 
    }
    
    /* character to search for (third parameter in command line) */
    c2c = argv[3][0];

    /* count the occurences of the given character */
    while ((cc = fgetc(fpr)) != EOF) if (cc == c2c) count++;
    
    /* close the file for reading */
    fclose(fpr);
    
    /* write the result in the output file */
    fprintf(fpw, "The character '%c' appears %d times in file %s.\n", c2c, count, argv[1]);
    
    /* close the output file */
    fclose(fpw);

    return 0; 
}
