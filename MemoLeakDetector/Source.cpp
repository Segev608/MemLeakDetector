#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <detours.h>
using namespace std;

#define Production 1

#define _RED		12
#define _YELLOW		14
#define _GREEN		10
#define _BLUE		9	
#define _DEFAULT	7

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// options declerations
char* exe = nullptr;
bool verbose = false;
bool output_flag = false;
const char* output = "diagnostic.txt";

int total_allocs	= 0;
int total_frees		= 0;
int total_reallocs	= 0;
size_t total_bytes	= 0;
const char* DLLPath;

struct Function
{
	string name;
	string address;
	size_t line_number = 0;
	string file;
	string _module;

	friend ostream& operator<<(ostream& os, const Function& f) {
		// one of the following formats:
		// <address>: <name>
		// <address>: <name> (<file>:<line>)
		// <address>: <name> (in <module>)
		os << "0x" << f.address << ": " << f.name;
		if (f.line_number) {
			const char* file_name = strrchr(f.file.c_str(), '\\');
			file_name = file_name ? file_name + 1 : f.file.c_str();

			os << " (" << file_name << ':' << f.line_number << ')';
		}
		else if (!f._module.empty()) {
			os << " (in " << f._module << ')';
		}
		return os;
	}
};

struct Call {
	string method;
	string address;
	size_t _size = 0;
	string last_address;
	vector<Function> trace;
};

enum class MemoryFunction { MALLOC, REALLOC, FREE, NOFUNC};
MemoryFunction hashStringFunction(string const& str) {
	if (str == "MALLOC") return MemoryFunction::MALLOC;
	if (str == "REALLOC") return MemoryFunction::REALLOC;
	if (str == "FREE") return MemoryFunction::FREE;
	return MemoryFunction::NOFUNC;
}

// main logic
bool run_exe();
void handle_output_path();
void analyze(ifstream&);
void new_call(vector<Call>&, const Call&);
//input & output
void print_trace(const Call&);
void parse_args(int, char**);
void print_results(vector<Call>);
template<class... Args>
void msg(char, bool, const char*, Args...);
// helpers
vector<string> split(string, char);
template<class T, class Pred>
size_t erase_if(vector<T>&, Pred);
void colorful_output(const char*, const int);

int main(int argc, char** argv)
{
	parse_args(argc, argv);

	cout << "\n ================== Process Output ==================\n";
	bool r = run_exe();
	cout << "\n ====================================================\n\n";
	if (!r)  
		msg('e', true, "Failed to run executable.\n");

	ifstream ofile(output);
	if (!ofile)
		msg('e', true, "Failed opening output file.\n");

	cout << "\n ===================== Analysis =====================\n\n";
	analyze(ofile);
	cout << "\n ====================================================\n\n";

	ofile.close();
	if (!output_flag) {
		remove(output);
	}

	cout << "\n ** We strongly recommend to compile your program in Debug mode ** \n";

	return 0;
}

void analyze(ifstream& ofile) {
	char tmp[256]{ 0 };
	while (!ofile.eof() && strcmp(tmp, "START") != 0)
		ofile.getline(tmp, 256);

	string func;
	vector<Call> calls;

	while (ofile >> func && func != "END")
	{
		Call c;
		c.method = func;
		switch (hashStringFunction(func)) {
		case MemoryFunction::MALLOC:
			ofile >> c.address >> c._size;
			break;
		case MemoryFunction::REALLOC:
			ofile >> c.last_address >> c._size >> c.address;
			break;
		case MemoryFunction::FREE:
			ofile >> c.address;
			break;
		}
		ofile.get();

		Function f;
		char buf[512]{ 0 };
		ofile.getline(buf, 512);
		while (buf[0])
		{
			auto result = split(buf, '|');
			f.name = result[0];
			f.address = result[1];
			f.file = result[2] == "NULL" ? "" : result[2];
			f.line_number = atoi(result[3].c_str());
			f._module = result[4] == "NULL" ? "" : result[4];
			c.trace.push_back(f);
			ofile.getline(buf, 512);
		}

		new_call(calls, c);
	}

	print_results(calls);
}

void new_call(vector<Call>& calls, const Call& call)
{
	vector<Call>::iterator c_it;
	size_t del;
	long long addr;
	long long new_addr;
	long long orig_addr;
	switch (hashStringFunction(call.method)) {
	case MemoryFunction::MALLOC:
		c_it = find_if(calls.begin(), calls.end(), [&](Call c) {
			orig_addr = strtoll(c.address.c_str(), nullptr, 16);
			new_addr = strtoll(call.address.c_str(), nullptr, 16);
			return orig_addr <= new_addr && new_addr < orig_addr + c._size;
			});
		if (c_it != calls.end()) {
			// Very Unlikly
			msg('w', false, "Memory collision");
			print_trace(call);
			printf("\tAddress 0x%s already in use", call.address.c_str());
			print_trace(*c_it);
			cout << '\n';
		}
		else
			calls.push_back(call);
		total_allocs++;
		total_bytes += call._size;
		break;
	case MemoryFunction::FREE:
		addr = strtoll(call.address.c_str(), nullptr, 16);
		if (addr == 0) {
			if (verbose) {
				msg('w', false, "Freed null pointer");
				print_trace(call);
				cout << '\n';
			}
		}
		else {
			del = erase_if(calls,
				[call](Call c) { return c.address == call.address; });
			if (del == 0) {
				msg('e', false, "Freed non-existing pointer");
				print_trace(call);
				printf("\tDid you free the same pointer twice?\n");
				colorful_output(" \tProcess Terminated.\n", _RED);
			}
		}
		total_frees++;
		break;
	case MemoryFunction::REALLOC:
		addr = strtoll(call.last_address.c_str(), nullptr, 16);
		if (addr == 0) {
			msg('e', false, "Reallocated null pointer");
			print_trace(call);
			colorful_output(" \tProcess Terminated.\n", _RED);
		}
		else {
			del = erase_if(calls,
				[call](Call c) { return c.address == call.last_address; });
			if (del == 0) {
				msg('e', false, "Reallocated non-existing pointer");
				print_trace(call);
				printf("\tDid you realloced a pointer you already freed?\n");
				colorful_output(" \tProcess Terminated.\n", _RED);
			}
			else
				calls.push_back(call);
		}
		total_reallocs++;
		total_bytes += call._size;
		break;
	}
}

void handle_output_path() {
	// validate output path, set env.
	ofstream f(output);
	if (!f)
		msg('e', true, "Cannot open or create %s.\n"
			"\tPlease make sure the file is not already open and"
			" that all directories in the path exist.\n", output);
	f.close();

	char env[256];
	sprintf_s(env, "MEMCHECK_PATH=%s", output);
	int res = _putenv(env);
	if (res != 0)
		msg('e', true, "Could not create environment variable.\n");
}

bool run_exe() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);

#if !Production
	DLLPath =
#ifdef _WIN64
		R"(..\x64\Debug\InjectedDLL.dll)";
#elif defined(_WIN32)
		R"(..\Debug\InjectedDLL.dll)";
#else
#error "Must define either _WIN32 or _WIN64".
#endif
#endif
	
	size_t l = strlen(exe) + 1;
	LPTSTR cmd = new wchar_t[l];
	mbstowcs_s(NULL, cmd, l, exe, l - 1);

	bool res = DetourCreateProcessWithDllEx(NULL, cmd,
			NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi, DLLPath, NULL);

	if (!res)
		return false;

	WaitForSingleObject(pi.hProcess, INFINITE);

	delete[] cmd;
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;
}

void parse_args(int argc, char** argv)
{
	if (argc < 2)
		msg('e', true, "Executable must be given\n");
	
	exe = argv[argc - 1];
	if (argc == 3)
	{
		if (strcmp(argv[1], "-v") == 0)
			verbose = true;
		else {
			output = argv[1];
			output_flag = true;
		}

	}
	else if (argc == 4) {
		verbose = true;
		output = argv[2];
		output_flag = true;
	}

#if Production
	char* p = strrchr(argv[0], '\\');
	p = p ? p + 1 : argv[0];
	p[0] = '\0';
	string path = argv[0];
	path +=
#ifdef _WIN64
		"..\\dll\\hookdll64.dll";
#elif defined(_WIN32)
		"..\\dll\\hookdll32.dll";
#else
#error "Must define either _WIN32 or _WIN64".
#endif

	DLLPath = _strdup(path.c_str());
#endif

	handle_output_path();
}

void colorful_output(const char* out, const int color) {
	SetConsoleTextAttribute(hConsole, color);
	cout << out;
	SetConsoleTextAttribute(hConsole, _DEFAULT);
}

vector<string> split(string s, char t) {
	size_t position = 0;
	vector<string> ret;
	do {
		position = s.find(t);
		ret.push_back(s.substr(0, position));
		s.erase(0, position + 1);
	} while (position != string::npos);

	return ret;
}

template<class T, class Pred>
size_t erase_if(vector<T>& v, Pred pred) {
	size_t old_size = v.size();
	auto new_end = remove_if(v.begin(), v.end(), pred);
	v.erase(new_end, v.end());
	return old_size - v.size();
}

void print_results(vector<Call> calls) {
	cout << "\n";
	size_t lost_bytes = 0;
	for (int i = 0; i < calls.size(); ++i) {
		lost_bytes += calls[i]._size;
		if (verbose) {
			printf(" %zu bytes are lost in loss record %d of %zu",
				calls[i]._size, i + 1, calls.size());
			print_trace(calls[i]);
		}
	}

	cout << "\n HEAP SUMMARY:\n";
	printf("\tin use at exit: %zu bytes\n", lost_bytes);
	printf("\ttotal heap usage: %d allocs, %d frees, ", total_allocs, total_frees);
	if (total_reallocs) printf("%d reallocs, ", total_reallocs);
	printf("%zu bytes allocated.\n", total_bytes);
}

void print_trace(const Call& c) {
	cout << '\n';
	for (auto it = c.trace.begin(); it < c.trace.end(); ++it)
		cout << (it == c.trace.begin() ? "\t\tat " : "\t\tby ") << *it << '\n';
}

template<class... Args>
void msg(char type, bool term, const char* fmt, Args... args) {
	cout << " ";
	switch (type)
	{
	case 'D':	// Default msg
	case 'd':
		printf(fmt, args...);
		break;
	case 'E':	// Error msg
	case 'e':
		colorful_output("Error: ", _RED);
		printf(fmt, args...);
		break;
	case 'W':	// Warning msg
	case 'w':
		colorful_output("Warning: ", _YELLOW);
		printf(fmt, args...);
		break;
	default:
		printf(fmt, args...);
		break;
	}

	if (term) {
		cout << "\n Aborting.\n";
		exit(0);
	}
}