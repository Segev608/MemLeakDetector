#include <stdlib.h>
#include <stdio.h>

int main(){
	const int loops = 10;
	const int allocation = 256;
	char* pointer = NULL;

	for(int i=0; i<loops; i++){
		// every loop here causes memory leak 
		printf("allocation no.%d\n", i);
		pointer = (char*)malloc(sizeof(char) * allocation);
	}
	free(pointer); // to prevent the leak - this "free" should be inside the loop
	return 0;
}
