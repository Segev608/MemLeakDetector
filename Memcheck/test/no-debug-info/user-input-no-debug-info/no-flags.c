#include <stdlib.h>
#include <stdio.h>
 
 void* alloc(){
	void* x = malloc(20);
	printf("%p\n", x);
	return x;
 }
 
 void sum(int* arr) {
	int s = 0;
	for(int i=0;i<5;++i){
		s += arr[i];
	}
	printf("%d", s);
 }
 
 int main() {
	int* x = (int*)alloc();
	int num;
	scanf("%d", &num);
	for(int i=0;i<5;++i){
		x[i] = i*num*10;
	}
	sum(x);
	return 0;
 }
 