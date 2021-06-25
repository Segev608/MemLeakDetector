
#include "pch.h"

MemAlloc origin_alloc = nullptr;
MemFree origin_free = nullptr;
MemRealloc origin_realloc = nullptr;

ofstream file;
vector<void*> in_use;
bool in_here = false;

bool IAThooking(HMODULE);
void log_trace(unsigned int);
PVOID CDECL new_alloc(size_t);
VOID CDECL new_free(PVOID);
PVOID CDECL new_realloc(PVOID, size_t);
template<class... Args>
void log_file(const char*, Args...);
PIMAGE_IMPORT_DESCRIPTOR getImportTable(HMODULE);

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
		DetourTransactionCommit();
		file.close();
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
		while (*(WORD*)pFirstThunk != 0 && *(WORD*)pOriginalFirstThunk != 0) //moving over IAT and over names' table
		{
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

	if (DetourTransactionCommit() != NO_ERROR) {
		printf(" Error: Could not hook memory managment functions.");
		exit(0);
	}

	return false;
}

void log_trace(unsigned int back = 1)
{
	const int MAX_STACK_COUNT = 64;
	const int BUFFER_SIZE = 256;
	void* stack[MAX_STACK_COUNT];
	unsigned short frames;
	char mod[BUFFER_SIZE]{ 0 };
	bool line_flag = false;
	HANDLE process;
	SYMBOL_INFO* symbol = (SYMBOL_INFO*)alloca(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
	IMAGEHLP_LINE64 line;
	HMODULE hModule = NULL;
	DWORD disp;
	process = GetCurrentProcess();

	ZeroMemory(symbol, sizeof(SYMBOL_INFO) +256 * sizeof(char));
	symbol->MaxNameLen = BUFFER_SIZE - 1;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	ZeroMemory(&line, sizeof(IMAGEHLP_LINE64));
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	// Initialize all debug information from executable (PE)
	SymInitialize(process, NULL, TRUE);

	// "Pull" frames from the stack trace
	frames = CaptureStackBackTrace(0, MAX_STACK_COUNT, stack, NULL);
	char call[512];
	for (unsigned int i = back; i < frames; ++i) {
		if (!SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
			continue;

		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)(symbol->Address), &hModule);
		if (hModule != NULL)
			GetModuleFileNameA(hModule, mod, BUFFER_SIZE);
		
		line_flag = SymGetLineFromAddr64(process, symbol->Address, &disp, &line);
		sprintf_s(call, 512, "%s|%p|%s|%d|%s\n",
			symbol->Name,
			(void*)(symbol->Address),
			line_flag ? line.FileName : "NULL",
			line_flag ? line.LineNumber : 0,
			hModule != NULL ? mod : "NULL");
		file << call;
		if (strcmp(symbol->Name, "main") == 0)
			break;
	}
	
	file << '\n';
	SymCleanup(process);
}

PVOID CDECL new_alloc(size_t _Size) {
	if (in_here) return origin_alloc(_Size);
	in_here = true;
	void* p = origin_alloc(_Size);
	log_file("MALLOC\t%p\t%zu\n", p, _Size);
	in_use.push_back(p);
	in_here = false;
	return p;
}

VOID CDECL new_free(PVOID ptr) {
	if (in_here) return origin_free(ptr);

	in_here = true;
	log_file("FREE\t%p\n", ptr);
	if (ptr != nullptr) {
		auto end = remove_if(in_use.begin(), in_use.end(), [ptr](void* p) {return p == ptr; });
		if (end == in_use.end())
			exit(0);
		in_use.erase(end, in_use.end());
	}
	origin_free(ptr);
	in_here = false;
	return;
}

PVOID CDECL new_realloc(PVOID ptr, size_t s)
{
	if (in_here) return origin_realloc(ptr, s);

	in_here = true;

	auto end = remove_if(in_use.begin(), in_use.end(), [ptr](void* p) {return p == ptr; });
	if (end == in_use.end()) {
		log_file("REALLOC\t%p\t%zu\t%p\n", ptr, s, 0);
		exit(0);
	}
	in_use.erase(end, in_use.end());

	void* ret = origin_realloc(ptr, s);
	log_file("REALLOC\t%p\t%zu\t%p\n", ptr, s, ret);
	in_use.push_back(ret);
	in_here = false;
	return ret;
}

template<class... Args>
void log_file(const char* fmt, Args... args)
{
	char output[256];
	sprintf_s(output, 256, fmt, args...);
	file << output;
	log_trace(3);
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
