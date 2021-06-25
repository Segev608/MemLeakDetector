#include <stdlib.h>

int main(){
	int* p = malloc(10);
	free(p);
	p = NULL;
	int* x = realloc(p, 100);
	return 0;
}
