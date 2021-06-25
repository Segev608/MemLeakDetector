#include <stdlib.h>
#include <stdio.h>

void welcome(){
	printf("Welocome to our Memcheck tool tutorial!\n");
}

void first_leak(){
	printf("Here is the first memory leak we'll see\n");
	int* ptr = malloc(55); /* memory leak of 55 bytes! */  
}

int main(){
	welcome();
	first_leak();
	return 0;
}
