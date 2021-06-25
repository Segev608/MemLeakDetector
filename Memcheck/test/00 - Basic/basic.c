#include <stdlib.h>
#include <stdio.h>

void first_leak(){
	int* ptr1 = malloc(11); /* memory leak of 11 bytes! */  
}

void second_leak(){
	int* ptr2 = malloc(22); /* memory leak of 22 bytes! */  
}

void third_leak(){
	int* ptr3 = malloc(33);
	free(ptr3);
}

int main(){
	first_leak();
	second_leak();
	third_leak();
	return 0;
}
