#include <Windows.h>
#include <string>
#include <iostream>
#include <algorithm>
using namespace std;

string output;
string exe;
bool verbose = false;

enum class Flag { HELP, OUTPUT, VERBOSE, NONE };
Flag hashStringFlag(string const& str) {
	if (str == "-h" || str == "--help") return Flag::HELP;
	if (str == "-o" || str == "--output") return Flag::OUTPUT;
	if (str == "-v" || str == "--verbose") return Flag::VERBOSE;
	return Flag::NONE;
}

bool is_x64(const char*);
void show_usage(const char*);
void parse_args(int, char**);

int main(int argc, char** argv)
{  
	parse_args(argc, argv);
	
	string cmd = argv[0];
	replace(cmd.begin(), cmd.end(), '/', '\\');
	size_t pos = cmd.rfind('\\');
	cmd.erase(pos == string::npos ? 0 : pos + 1);

	try {
		cmd += is_x64(exe.c_str()) ?
			//"bin\\memcheck64.exe" : "bin\\memcheck32.exe";
			"..\\x64\\Debug\\MemoLeakDetector.exe" : "..\\Debug\\MemoLeakDetector.exe";
	}
	catch (const char*){
		printf(" Error: Executable not found.\n");
		exit(0);
	}

	if (verbose) cmd += " -v";
	if (!output.empty()) cmd += " \"" + output + '"';
	cmd += " \"" + exe + '"';
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);

	size_t l = cmd.length() + 1;
	wchar_t* command = new wchar_t[l];
	mbstowcs_s(NULL, command, l, cmd.c_str(), l-1);
	
	CreateProcess(NULL, command,
		NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi);
	
	WaitForSingleObject(pi.hProcess, INFINITE);

	delete[] command;
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

}

bool is_x64(const char* executable) {
	DWORD res;
	if (!GetBinaryTypeA(executable, &res))
		throw "File not Found.";
	return res == SCS_64BIT_BINARY;
}

void parse_args(int argc, char** argv)
{
	if (argc < 2)
		show_usage(argv[0]);

	//parsing
	for (int i = 1; i < argc; ++i) {
		switch (hashStringFlag(argv[i])) {
		case Flag::HELP:
			show_usage(argv[0]);
			break;
		case Flag::OUTPUT:
			if (i + 1 < argc) 
				output = argv[++i];
			break;
		case Flag::VERBOSE:
			verbose = true;
			break;
		case Flag::NONE:
			if (i == argc - 1)
				exe = argv[i];
			else
				cout << " Unknown option: \"" << argv[i] << "\". Ignoring.\n";
			break;
		}
	}

	if (exe.empty()) {
		cout << " Error: Executable must be given\n";
		show_usage(argv[0]);
	}
}

void show_usage(const char* name)
{
	cerr << "Usage: " << name << " <options> EXECUTABLE\n"
		<< "Options:\n"
		<< "\t-h, --help\t\tShow this help message\n"
		<< "\t-o, --output OUTPUT\tSpecify the output path for a file that will contain all heap usage information\n"
		<< "\t-v, --verbose\t\tRetraive warnings and more information about the leakes"
		<< endl;

	exit(0);
}
