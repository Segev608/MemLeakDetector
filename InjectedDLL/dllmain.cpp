#include "pch.h"


typedef PVOID(CDECL* MemAlloc)(size_t);
typedef VOID(CDECL* MemFree)(PVOID);

MemAlloc origin_malloc;
MemFree origin_free;

ofstream file;

bool IAThooking(HMODULE hInstance, LPCSTR targetFunction, PVOID newFunc)
{
	bool flag = false;

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
		while (*(WORD*)pFirstThunk != 0 && *(WORD*)pOriginalFirstThunk != 0) //moving over IAT and over names' table
		{
			if (std::string(pFuncData->Name).compare(TARGET_ALLOCATION) == 0)//checks if we are in the malloc Function
			{
				//cout << pFuncData->Name << endl;
				//cout << (char*)((PBYTE)hInstance + importedModule->Name) << endl;
				origin_malloc = (MemAlloc)DetourFindFunction((char*)((PBYTE)hInstance + importedModule->Name), pFuncData->Name);
				DetourAttach(&(PVOID&)origin_malloc, new_malloc);
			}
			else if (std::string(pFuncData->Name).compare(TARGET_DEALLOCATION) == 0)//checks if we are in the malloc Function
			{
				//cout << pFuncData->Name << endl;
				//cout << (char*)((PBYTE)hInstance + importedModule->Name) << endl;
				origin_free = (MemFree)DetourFindFunction((char*)((PBYTE)hInstance + importedModule->Name), pFuncData->Name);
				DetourAttach(&(PVOID&)origin_free, new_free);
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

PVOID CDECL new_malloc(size_t _Size) {
	void* p = origin_malloc(_Size);
	char output[256];
	sprintf_s(output, 256, "[+]\t%p\t%zu\r\n", p, _Size);
	file.write(output, strlen(output) + 1);
	return p;
}
VOID CDECL new_free(PVOID ptr) {
	char output[256];
	sprintf_s(output, 256, "[-]\t%p\r\n", ptr);
	file.write(output, strlen(output) + 1);

	origin_free(ptr);
	return;
}

PIMAGE_IMPORT_DESCRIPTOR getImportTable(HMODULE hInstance)
{
	PIMAGE_DOS_HEADER dosHeader;
	IMAGE_OPTIONAL_HEADER optionalHeader;
	PIMAGE_NT_HEADERS ntHeader;
	IMAGE_DATA_DIRECTORY dataDirectory;

	dosHeader = (PIMAGE_DOS_HEADER)hInstance;//cast hInstance to (IMAGE_DOS_HEADER *) - the MZ Header
	ntHeader = (PIMAGE_NT_HEADERS)((PBYTE)dosHeader + dosHeader->e_lfanew);//The PE Header begin after the MZ Header (which has size of e_lfanew)
	optionalHeader = (IMAGE_OPTIONAL_HEADER)(ntHeader->OptionalHeader); //Getting OptionalHeader
	dataDirectory = (IMAGE_DATA_DIRECTORY)(optionalHeader.DataDirectory[IMPORT_TABLE_OFFSET]);//Getting the import table of DataDirectory
	return (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)hInstance + dataDirectory.VirtualAddress);//ImageBase+RVA to import table

}
bool rewriteThunk(PIMAGE_THUNK_DATA pThunk, void* newFunc)
{
	DWORD CurrentProtect;
	DWORD junk;
	VirtualProtect(pThunk, 4096, PAGE_READWRITE, &CurrentProtect);//allow write to the page
	DWORD sourceAddr = pThunk->u1.Function;
	pThunk->u1.Function = (DWORD)newFunc; // rewrite the IAT to new function
	VirtualProtect(pThunk, 4096, CurrentProtect, &junk);//return previous premissions
	return true;
}


BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	if (DetourIsHelperProcess())
		return true;


	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();
		file.open("C:\\Users\\User\\Desktop\\diagnostic.txt", ofstream::trunc);

		IAThooking(GetModuleHandleA(NULL), TARGET_ALLOCATION, new_malloc);

		break;
	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)origin_malloc, new_malloc);
		DetourDetach(&(PVOID&)origin_free, new_free);
		if (DetourTransactionCommit() == NO_ERROR)
			printf("undetoured successfully\n");
		else
			printf("undetoured unsuccessfully\n");
		file.close();

		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

