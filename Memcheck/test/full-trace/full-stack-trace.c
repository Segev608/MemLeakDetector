#include <stdlib.h>
#include <stdio.h>

void* f1(){ return malloc(50); }

void* f2(){ return f1(); }

void* f3(){ return f2(); }

void* f4(){ return f3(); }

void* f5(){ return f4(); }

void main(){
	void* x = f5();
	printf("Hello World");
}
