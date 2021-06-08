#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <iostream>
#include <stdlib.h>
#pragma warning(push)
#pragma warning(disable : 4091)
#include "DbgHelp.h"
#pragma comment(lib, "DbgHelp.lib")
#pragma warning(pop)
using namespace std;

#define Debug 0
#if Debug == 1

#include <fstream>
#include <Windows.h>
#include <detours.h>

#define Halloc 0
#if Halloc == 0

#define TARGET_ALLOCATION "malloc"
#define TARGET_DEALLOCATION "free"
#define TARGET_REALLOCATION "realloc"

typedef PVOID(CDECL* MemAlloc)(size_t);
typedef VOID(CDECL* MemFree)(PVOID);
typedef PVOID(CDECL* MemRealloc)(PVOID, size_t);

PVOID CDECL new_alloc(size_t _Size);
VOID CDECL new_free(PVOID _Size);
PVOID CDECL new_realloc(PVOID, size_t);

#else
#define TARGET_ALLOCATION "HeapAlloc"
#define TARGET_DEALLOCATION "HeapFree"

// typedefs
typedef LPVOID(WINAPI* MemAlloc)(HANDLE, DWORD, SIZE_T);
typedef BOOL(WINAPI* MemFree)(HANDLE, DWORD, LPVOID);
typedef VOID(CDECL* mMemFree)(PVOID);

//new declerations
LPVOID WINAPI new_alloc(HANDLE, DWORD, SIZE_T);
BOOL WINAPI new_free(HANDLE, DWORD, LPVOID);
VOID CDECL new_mfree(PVOID);
#endif

MemAlloc origin_alloc;
MemFree origin_free;
MemRealloc origin_realloc;
//mMemFree origin_mfree;

ofstream file;

#if Halloc == 0
PVOID CDECL new_alloc(size_t _Size) {
	void* p = origin_alloc(_Size);
	char output[256];
	sprintf_s(output, 256, "ALLOC:\t%p\t%zu\r\n", p, _Size);
	file.write(output, strlen(output) + 1);
	return p;
}
VOID CDECL new_free(PVOID ptr) {
	char output[256];
	sprintf_s(output, 256, "FREE:\t%p\r\n", ptr);
	file.write(output, strlen(output) + 1);

	origin_free(ptr);
	return;
}

PVOID CDECL new_realloc(PVOID ptr, size_t s)
{
	void* ret = origin_realloc(ptr, s);
	char output[256];
	sprintf_s(output, 256, "REALLOC\t%p\t%zu -> %p\r\n", ptr, s, ret);
	file.write(output, strlen(output) + 1);

	return ret;
}
#else

LPVOID WINAPI new_alloc(HANDLE h, DWORD d, SIZE_T s) {
	static bool isRunning = false;
	void* p = origin_alloc(h, d, s);
	if (isRunning)
		return p;
	isRunning = true;
	char output[256];
	sprintf_s(output, 256, "[+]\t%p\t%zu\r\n", p, s);
	//fprintf(file, "[+]\t%p\t%zu\r\n", p, s);
	file.write(output, strlen(output) + 1);
	isRunning = false;
	return p;
}

BOOL WINAPI new_free(HANDLE h, DWORD d, LPVOID ptr) {
	char output[256];
	sprintf_s(output, 256, "[-]\t%p\r\n", ptr);
	//fprintf(file, "[-]\t%p\r\n", ptr);
	file << output;
	return origin_free(h, d, ptr);
}

VOID CDECL new_mfree(PVOID ptr)
{
	char output[256];
	sprintf_s(output, 256, "[-]\t%p\r\n", ptr);
	file.write(output, strlen(output) + 1);

	origin_mfree(ptr);
	return;
}
#endif

PIMAGE_IMPORT_DESCRIPTOR getImportTable(HMODULE hInstance)
{
	PIMAGE_DOS_HEADER dosHeader;
	IMAGE_OPTIONAL_HEADER optionalHeader;
	PIMAGE_NT_HEADERS ntHeader;
	IMAGE_DATA_DIRECTORY dataDirectory;

	dosHeader = (PIMAGE_DOS_HEADER)hInstance;//cast hInstance to (IMAGE_DOS_HEADER *) - the MZ Header
	ntHeader = (PIMAGE_NT_HEADERS)((PBYTE)dosHeader + dosHeader->e_lfanew);//The PE Header begin after the MZ Header (which has size of e_lfanew)
	optionalHeader = (IMAGE_OPTIONAL_HEADER)(ntHeader->OptionalHeader); //Getting OptionalHeader
	dataDirectory = (IMAGE_DATA_DIRECTORY)(optionalHeader.DataDirectory[1]);//Getting the import table of DataDirectory
	return (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)hInstance + dataDirectory.VirtualAddress);//ImageBase+RVA to import table

}

bool IAThooking(HMODULE hInstance)
{

	PIMAGE_IMPORT_DESCRIPTOR importedModule;
	PIMAGE_THUNK_DATA pFirstThunk, pOriginalFirstThunk;
	PIMAGE_IMPORT_BY_NAME pFuncData;

	importedModule = getImportTable(hInstance);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	while (*(WORD*)importedModule != 0) //over on the modules (DLLs)
	{

		pFirstThunk = (PIMAGE_THUNK_DATA)((PBYTE)hInstance + importedModule->FirstThunk);//pointing to its IAT
		pOriginalFirstThunk = (PIMAGE_THUNK_DATA)((PBYTE)hInstance + importedModule->OriginalFirstThunk);//pointing to OriginalThunk
		pFuncData = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hInstance + pOriginalFirstThunk->u1.AddressOfData);// and to IMAGE_IMPORT_BY_NAME
		cout << (char*)((PBYTE)hInstance + importedModule->Name) << endl;
		while (*(WORD*)pFirstThunk != 0 && *(WORD*)pOriginalFirstThunk != 0) //moving over IAT and over names' table
		{
			cout << '\t' << pFuncData->Name << endl;
			if (std::string(pFuncData->Name).compare(TARGET_ALLOCATION) == 0)//checks if we are in the malloc Function
			{
				//cout << pFuncData->Name << endl;
				//cout << (char*)((PBYTE)hInstance + importedModule->Name) << endl;
				origin_alloc = (MemAlloc)DetourFindFunction((char*)((PBYTE)hInstance + importedModule->Name), pFuncData->Name);
				long r = DetourAttach(&(PVOID&)origin_alloc, new_alloc);
			}
			else if (std::string(pFuncData->Name).compare(TARGET_DEALLOCATION) == 0)//checks if we are in the malloc Function
			{
				//cout << pFuncData->Name << endl;
				//cout << (char*)((PBYTE)hInstance + importedModule->Name) << endl;
				origin_free = (MemFree)DetourFindFunction((char*)((PBYTE)hInstance + importedModule->Name), pFuncData->Name);
				long r = DetourAttach(&(PVOID&)origin_free, new_free);
			}
			else if (std::string(pFuncData->Name).compare(TARGET_REALLOCATION) == 0)
			{
				origin_realloc = (MemRealloc)DetourFindFunction((char*)((PBYTE)hInstance + importedModule->Name), pFuncData->Name);
				long r = DetourAttach(&(PVOID&)origin_realloc, new_realloc);
				cout << r;
			}

			pOriginalFirstThunk++; // next node (function) in the array
			pFuncData = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hInstance + pOriginalFirstThunk->u1.AddressOfData);
			pFirstThunk++;// next node (function) in the array
		}
		importedModule++; //next module (DLL)
	}

	if (DetourTransactionCommit() == NO_ERROR)
		printf("detoured successfully\n");
	else
		printf("detoured un-successfully\n");

	return false;
}
#endif

void printStack(void)
{
	static const int MAX_STACK_COUNT = 64;
	void* stack[MAX_STACK_COUNT];
	unsigned short frames;
	SYMBOL_INFO* symbol;
	HANDLE process;

	process = GetCurrentProcess();

	SymInitialize(process, NULL, TRUE);

	frames = CaptureStackBackTrace(0, 20, stack, NULL);
	cout << frames << '\n';
	for (int i = 0; i < 20; ++i)
		cout << stack[i] << '\n';

	symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	IMAGEHLP_LINE64* line;
	line = (IMAGEHLP_LINE64*)malloc(sizeof(IMAGEHLP_LINE64));
	line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	HMODULE hModule;
	

	printf("=========call stack==========\n");
	for (int i = 0; i < 20; i++)
	{
		char mod[256]{ 0 };
		SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
		DWORD disp;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)(symbol->Address), &hModule);
		if (hModule != NULL)GetModuleFileNameA(hModule, mod, 256);
		if (SymGetLineFromAddr64(process, symbol->Address, &disp, line))
			printf("%i: (%s)%s(%d)(%s) - 0x%0llX\n", frames - i - 1, line->FileName,symbol->Name, line->LineNumber, mod,symbol->Address);
		else
			printf("%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name, symbol->Address);
	}
	printf("=============================\n");

	free(symbol);
}

void* bar() {
	cout << "asdf\n";
	printStack();
	return malloc(365);
}

void foo() {
	bar();
	cout << "Foo\n";
}

int main() {
#if Debug == 1
	file.open("C:\\diagnostic.txt", ofstream::trunc);
	//fopen_s(&file, "C:\\test.txt", "w");
	IAThooking(GetModuleHandleA(NULL));
	//fprintf(file, "START\n");
	file << "START\n";
#endif
	// =================== CODE =====================
	foo();
#if Debug == 1
	//fprintf(file, "END\n");
	file << "END\n";
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)origin_alloc, new_alloc);
	DetourDetach(&(PVOID&)origin_free, new_free);
	DetourDetach(&(PVOID&)origin_realloc, new_realloc);
	if (DetourTransactionCommit() == NO_ERROR)
		printf("undetoured successfully\n");
	else
		printf("undetoured unsuccessfully\n");
	//fclose(file);
	file.close();
#endif

	return 0;
}