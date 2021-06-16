#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <string>
using namespace std;

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
	//printf("Hello World!");
	//void* p = malloc(20);
	//foo();
	//free(p);
	//allocate();
	//free(allocate()); 
	//bar(); 
	//free(p); 
	string cmd = "C:/Windows/System32/vcruntime140d.dll";
	replace(cmd.begin(), cmd.end(), '/', '\\');
	size_t pos = cmd.rfind('\\');
	cmd.erase(pos == string::npos ? 0 : pos + 1);
	cout << cmd;
	//printf("I wont be printed!");
	return 0;
}