#include <stdlib.h>
#include <stdio.h>
 
 int main() {
	int* x = (int*)malloc(20);
	void* y = malloc(50);
	printf("HELLO!!!!!!\n%p\t%p\n", x, y);
	//free(y);
	return 0;
 }
 
