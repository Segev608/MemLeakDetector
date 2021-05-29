#include <malloc.h>
#include <crtdbg.h>
#include <stdio.h>


int main() {
	char* x = (char*)malloc(10);
	free(x);
	return 0;
}