// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    void* p = NULL;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        printf("Entered dll\n");
        p = malloc(70);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        printf("Exiting dll\n");
        //free(p);   <--- problem
        printf("LEAK\n");
        break;
    }
    return TRUE;
}

