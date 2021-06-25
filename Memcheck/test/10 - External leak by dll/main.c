#include <windows.h> 
#include <stdio.h> 

int main() {
    void* x = malloc(50); // <-- Leak here

    HINSTANCE hinstLib;
    // Get a handle to the DLL module.
    hinstLib = LoadLibrary(TEXT("dllmain.dll"));
	return 0;
}