#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <detours.h>
using namespace std;

#define _RED		12
#define _YELLOW		14
#define _GREEN		10
#define _BLUE		9	
#define _DEFAULT	7

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// TODO ? :
// - see line 320
// - support x86/x64
// - get full call stack (until main)
// - hook new/new[] and delete/delete[] (support for c++)

// options declerations
bool output_flag = false;
char* output = (char*)"C:\\diagnostic.txt";
char* exe = nullptr;
bool verbose = false;

int total_allocs = 0;
int total_frees = 0;
int total_reallocs = 0;
int total_bytes = 0;

struct Call {
	string method;
	string address;
	size_t _size = 0;
	string last_address;
	string caller;
	string caller_addr;
	size_t caller_line = 0;
	string file;
	string _module;
};

void analyze(ifstream& ofile);
vector<string> split(string s, char t);
HANDLE run_exe();
LPTSTR get_exe();
bool is_x64(char*);
void show_usage(string);
void parse_args(int, char**);
void handle_output_path();
void new_call(vector<Call>& calls, const Call& call);
template<class T, class Pred>
int erase_if(vector<T>& v, Pred pred);
void print_results(vector<Call>);
void colorful_output(const char* out, const int color);

int main(int argc, char** argv)
{

	//parse_args(argc, argv);
	exe = (char*)"..\\Debug\\TestSamples.exe";
	handle_output_path();

	HANDLE hp = run_exe();
	WaitForSingleObject(hp, INFINITE);

	// parse file
	ifstream ofile(output);
	if (!ofile) {
		cout << "ERROR opening file";
		exit(0);
	}

	analyze(ofile);

	ofile.close();
	if (!output_flag) {
		remove(output);
	}

	return 0;
}

void analyze(ifstream& ofile) {
	char tmp[256]{ 0 };
	while (strcmp(tmp, "START") != 0)
		ofile.getline(tmp, 256);

	string func;
	string line;
	vector<Call> calls;

	while (ofile >> func)
	{
		if (func == "END") break;

		Call c;
		c.method = func;

		if (func == "MALLOC")
			ofile >> c.address >> c._size;
		else if (func == "FREE")
			ofile >> c.address;
		else if (func == "REALLOC")
			ofile >> c.last_address >> c._size >> c.address;

		char buf[1000]{ 0 };
		ofile.get();
		ofile.getline(buf, 1000);
		line = buf;

		if (!line.empty()) {
			auto result = split(line, '|');
			c.caller = result[0];
			c.caller_addr = result[1];
			c.file = result[2] == "NULL" ? "" : result[2];
			c.caller_line = atoi(result[3].c_str());
			c._module = result[4] == "NULL" ? "" : result[4];
		}

		new_call(calls, c);
	}

	print_results(calls);
}

void new_call(vector<Call>& calls, const Call& call)
{
	if (call.method == "MALLOC") {
		auto end = find_if(calls.begin(), calls.end(), [call](Call c) {
			char* p;
			long orig_addr = strtol(c.address.c_str(), &p, 16);
			long new_addr = strtol(call.address.c_str(), &p, 16);
			return orig_addr <= new_addr && new_addr < orig_addr + c._size;
			});
		if (end != calls.end()) {
			cout << "memory collision\n";
			// problem
		}
		else
			calls.push_back(call);
		total_allocs++;
		total_bytes += call._size;
	}
	else if (call.method == "FREE") {
		char* p;
		long addr = strtol(call.address.c_str(), &p, 16);
		if (addr == 0) {
			cout << "freed null pointer\n";
			//freed null pointer
		}
		else {
			int del = erase_if(calls,
				[call](Call c) { return c.address == call.address; });
			if (del == 0) {
				cout << "freed non-existing pointer\n";
				// problem 
			}
		}
		total_frees++;

	}
	else if (call.method == "REALLOC") {
		int del = erase_if(calls,
			[call](Call c) { return c.address == call.last_address; });
		if (del == 0) {
			cout << "reallocated non-existing pointer\n";
			// problem 
		}
		else
			calls.push_back(call);
		total_reallocs++;
		total_bytes += call._size;

	}

}

void handle_output_path() {
	// validate output path, set env etc.

	char env[256];
	sprintf_s(env, "MEMCHECK_PATH=%s", output);
	int res = _putenv(env);
	if (res != 0) {
		cout << "ERROR putenv";
		exit(0);
	}
}

HANDLE run_exe() {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);

	char* DLLPath = (char*)R"(..\Debug\InjectedDLL.dll)";

	bool res = DetourCreateProcessWithDllEx(NULL, get_exe(),
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi, DLLPath, NULL);
	if (!res) {
		cout << "Failed to run executable";
		exit(0);
	}

	ResumeThread(pi.hThread);

	return pi.hProcess;
	//return ?
}

LPTSTR get_exe() {
	// check exists, convert to LPTSTR
	ifstream f(exe);
	if (!f) {
		cout << "Executable not found.\n";
		exit(0);
	}
	f.close();

	int l = strlen(exe) + 1;
	size_t s;
	wchar_t* buf = new wchar_t[l];
	mbstowcs_s(&s, buf, l, exe, l);
	return buf;
}

bool is_x64(char* executable) {
	DWORD res;
	if (!GetBinaryTypeA(executable, &res))
		throw "File not Found.";
	return res == SCS_64BIT_BINARY;
}

void show_usage(std::string name)
{
	std::cerr << "Usage: " << name << " <options> EXECUTABLE\n"
		<< "Options:\n"
		<< "\t-h, --help\t\tShow this help message\n"
		<< "\t-o, --output OUTPUT\tSpecify the output path\n"
		<< "\t-v, --verbose\tRetraive more information"
		<< std::endl;

	exit(0);
}

void parse_args(int argc, char** argv)
{
	if (argc < 2)
		show_usage(argv[0]);

	//parsing
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if ((arg == "-h") || (arg == "--help"))
			show_usage(argv[0]);
		// options tower
		else if ((arg == "-o") || (arg == "--output")) {
			if (i + 1 < argc) {
				output = argv[++i];
				output_flag = true;
			}
		}
		else if (((arg == "-v") || (arg == "--verbose"))) {
			verbose = true;
		}
		else {
			if (i == argc - 1)
				exe = argv[i];
			else
				show_usage(argv[0]);
		}
	}

	handle_output_path();
	if (!exe) {
		cout << "Executable must be given\n";
		show_usage(argv[0]);
	}
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
int erase_if(vector<T>& v, Pred pred) {
	int old_size = v.size();
	auto new_end = remove_if(v.begin(), v.end(), pred);
	v.erase(new_end, v.end());
	return old_size - v.size();
}

void print_results(vector<Call> calls) {
	cout << "\n";
	int lost_bytes = 0;
	for (int i = 0; i < calls.size(); ++i) {
		lost_bytes += calls[i]._size;
		if (verbose) {
			// TODO CHECKS ON GIVEN DATA & OUTPUT ACCORDINGLY
			printf(" %d bytes are lost in loss record %d of %d\n", calls[i]._size, i + 1, calls.size());
			const char* file_name = strrchr(calls[i].file.c_str(), '\\');
			printf("\tat 0x%s: %s (%s:%d)\n", calls[i].caller_addr.c_str(), calls[i].caller.c_str(), file_name + 1, calls[i].caller_line);
		}
	}

	cout << "\n HEAP SUMMARY:\n";
	printf("\tin use at exit: %d bytes\n", lost_bytes);
	printf("\ttotal heap usage: %d allocs, %d frees, ", total_allocs, total_frees);
	if (total_reallocs) printf("%d reallocs, ", total_reallocs);
	printf("%d bytes allocated.\n", total_bytes);
}
