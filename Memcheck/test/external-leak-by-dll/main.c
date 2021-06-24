#include <windows.h> 
#include <stdio.h> 

typedef int(__cdecl* MYPROC)(LPWSTR);

int main() {
    void* x = malloc(50);

    HINSTANCE hinstLib;
    MYPROC ProcAdd;
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
    // Get a handle to the DLL module.
    hinstLib = LoadLibrary(TEXT("DllWithLeak.dll"));
    // If the handle is valid, try to get the function address.
    //if (hinstLib != NULL){
    //    ProcAdd = (MYPROC)GetProcAddress(hinstLib, "myPuts");
    //    // If the function address is valid, call the function.
    //    if (NULL != ProcAdd){
    //        fRunTimeLinkSuccess = TRUE;
    //        (ProcAdd)(L"Message sent to the DLL function\n");
    //    }
    //    // Free the DLL module.
    //    fFreeResult = FreeLibrary(hinstLib);
    //}
    // If unable to call the DLL function, use an alternative.
	return 0;
}