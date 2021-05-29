#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <detours.h>
using namespace std;

int main(int argc, char** argv)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	char* DLLPath = (char*)R"(C:\Users\User\source\repos\MemoLeakDetector\Debug\InjectedDLL.dll)"; //This will contain the path to the DLL to inject
	LPTSTR executable = _tcsdup(LR"(C:\Users\User\source\repos\MemoLeakDetector\Debug\TestSamples.exe)");

	DetourCreateProcessWithDllEx(NULL, executable, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi, DLLPath, NULL);
	ResumeThread(pi.hThread);

	return 0;
}