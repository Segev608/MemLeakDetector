#include <stdlib.h>

int main(){
	int* p = malloc(60);
	free(p); // <-- good practice
	free(p); // <-- bad!, freed null pointer. Invalid operation!
	return 0;
}
