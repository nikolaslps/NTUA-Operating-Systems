#include <stdio.h>
#include <unistd.h>
#include "zing2.h"

void zing(void){
	char *user = getlogin();
	if (user == NULL)
	printf("error\n");
	else
	printf("Hello 2: %s\n", user);
}
