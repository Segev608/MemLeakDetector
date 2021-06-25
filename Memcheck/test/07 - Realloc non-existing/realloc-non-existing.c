#include <stdlib.h>

int main(){
	int* p = malloc(50);
	free(p);
	// 'til here, everything fine
	
	int y = 5;
	int* x = realloc(&y, 50); // trying to realloc non existing pointer 
	return 0;
}
