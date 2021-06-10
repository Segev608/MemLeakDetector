
#include "pch.h"
#include "InjectedDll.h"
#pragma warning(push)
#pragma warning(disable : 4091)
#include "DbgHelp.h"
#pragma comment(lib, "DbgHelp.lib")
#pragma warning(pop)

MemAlloc origin_alloc = nullptr;
MemFree origin_free = nullptr;
MemRealloc origin_realloc = nullptr;

ofstream file;

SYMBOL_INFO* symbol; 
IMAGEHLP_LINE64* line;

char* get_last_call_info(int back = 1)
{
	const int MAX_STACK_COUNT = 64;
	const int BUFFER_SIZE = 256;
	void* stack[MAX_STACK_COUNT];
	unsigned short frames;
	char mod[BUFFER_SIZE]{ 0 };
	bool line_flag = false;
	HANDLE process;
	
	HMODULE hModule = NULL;
	DWORD disp;

	process = GetCurrentProcess();

	ZeroMemory(symbol, sizeof(SYMBOL_INFO) + 256 * sizeof(char));
	symbol->MaxNameLen = BUFFER_SIZE - 1;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	ZeroMemory(line, sizeof(IMAGEHLP_LINE64));
	line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	// Initialize all debug information from executable (PE)
	SymInitialize(process, NULL, TRUE);

	// "Pull" 20 frames from the stack trace
	frames = CaptureStackBackTrace(0, MAX_STACK_COUNT, stack, NULL);
	if (frames < back + 1)
		return (char*)"";

	// get symbol from address
	if (!SymFromAddr(process, (DWORD64)(stack[back]), 0, symbol))
		return (char*)"";

	// module name
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)(symbol->Address), &hModule);
	if (hModule != NULL)
		GetModuleFileNameA(hModule, mod, BUFFER_SIZE);

	line_flag = SymGetLineFromAddr64(process, symbol->Address, &disp, line);

	char ret[1000];
	sprintf_s(ret, "%s|%08llX|%s|%d|%s",
		symbol->Name, 
		symbol->Address,
		line_flag ? line->FileName : "NULL",
		line_flag ? line->LineNumber : 0,
		hModule != NULL ? mod : "NULL");

	//free(symbol);
	//if (line) free(line);

	return ret;
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	if (DetourIsHelperProcess())
		return true;
	size_t s = 1;
	char buf[256];
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();
		symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
		line = (IMAGEHLP_LINE64*)malloc(sizeof(IMAGEHLP_LINE64));
		getenv_s(&s, buf, "MEMCHECK_PATH");
		file.open(buf, ofstream::trunc);

		IAThooking(GetModuleHandleA(NULL));
		file << "START\n";
		break;
	case DLL_PROCESS_DETACH:
		file << "END\n";
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		if (origin_alloc)	DetourDetach(&(PVOID&)origin_alloc, new_alloc);
		if (origin_free)	DetourDetach(&(PVOID&)origin_free, new_free);
		if (origin_realloc) DetourDetach(&(PVOID&)origin_realloc, new_realloc);

		if (DetourTransactionCommit() == NO_ERROR)
			printf("undetoured successfully\n");
		else
			printf("undetoured unsuccessfully\n");
		file.close();
		free(symbol);
		free(line);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
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
		char* module_name = (char*)((PBYTE)hInstance + importedModule->Name);
		// cout << module_name << '\n';
		while (*(WORD*)pFirstThunk != 0 && *(WORD*)pOriginalFirstThunk != 0) //moving over IAT and over names' table
		{
			// cout << '\t' << pFuncData->Name << endl;
			if (std::string(pFuncData->Name).compare(TARGET_ALLOCATION) == 0)//checks if we are in the malloc Function
			{
				origin_alloc = (MemAlloc)DetourFindFunction(module_name, pFuncData->Name);
				DetourAttach(&(PVOID&)origin_alloc, new_alloc);
			}
			else if (std::string(pFuncData->Name).compare(TARGET_DEALLOCATION) == 0)//checks if we are in the malloc Function
			{
				origin_free = (MemFree)DetourFindFunction(module_name, pFuncData->Name);
				DetourAttach(&(PVOID&)origin_free, new_free);
			}
			else if (std::string(pFuncData->Name).compare(TARGET_REALLOCATION) == 0)
			{
				origin_realloc = (MemRealloc)DetourFindFunction(module_name, pFuncData->Name);
				DetourAttach(&(PVOID&)origin_realloc, new_realloc);
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

PVOID CDECL new_alloc(size_t _Size) {
	
	void* p = origin_alloc(_Size);
	
	log_file("MALLOC\t%p\t%zu\n", p, _Size);
	return p;
}
VOID CDECL new_free(PVOID ptr) {
	log_file("FREE\t%p\n", ptr);
	origin_free(ptr);
	return;
}

PVOID CDECL new_realloc(PVOID ptr, size_t s)
{
	void* ret = origin_realloc(ptr, s);
	log_file("REALLOC\t%p\t%zu\t%p\n", ptr, s, ret);
	return ret;
}

template<class... Args>
void log_file(const char* fmt, Args... args)
{
	char* lc = get_last_call_info(3);
	char output[256];
	sprintf_s(output, 256, fmt, args...);
	file << output << lc << '\n';
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


