#include <stdlib.h>
#include <stdio.h>

void foo() {
	void *x = malloc(10);
	void* p = realloc(x, 100);
	free(p ? p : x);
}

void* allocate() {
	return malloc(50);
}

void bar() {
	free(nullptr);
}

int main(int argc, char** argv) {
	printf("Hello World!");
	foo();
	return 0;
}