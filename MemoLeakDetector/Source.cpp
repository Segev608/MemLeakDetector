#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>
#include <fstream>
#include <detours.h>
using namespace std;

#define _RED		12
#define _YELLOW		14
#define _GREEN		10
#define _BLUE		9	
#define _DEFAULT	7

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
void restoreAttribute();
void colorful_output(const char* out, const int color);

// options declerations
bool output_flag = false;
char* output = (char*)"diagnostic.txt";
char* exe = (char*)"..\\Debug\\TestSamples.exe";
bool debug = false;

HANDLE run_exe();
LPTSTR get_exe();
bool is_x64(char*);
void show_usage(string);
void parse_args(int, char**);
void handle_output_path();
void show_details(string args[5]);
bool parse_headline(ifstream& ofile, const string& row, char(&addr)[9], int& sz, int& new_size, char(&new_addr)[9]);

int main(int argc, char** argv)
{

    parse_args(argc, argv);
    handle_output_path();

    HANDLE hp = run_exe();
    WaitForSingleObject(hp, INFINITE);

    // parse file
    ifstream ofile(output);
    if (!ofile) {
        cout << "ERROR";
        exit(0);
    }

    string func;
    char line[256] { 0 };
    while (strcmp(line, "START") != 0)
        ofile.getline(line, 256);

    bool details = false;
    char addr[9];
    int sz;
    char new_addr[9];
    int new_size;

    size_t position;
    string tokens[5];
    auto reset = [](string args[5]) {for (int i = 0; i < 5; i++) args[i].clear(); };

    while (ofile >> func)
    {
        if (!details) {
            if (parse_headline(ofile, func, addr, sz, new_size, new_addr))
                break;
        }
        else {
            if (func != "") {
                for (int i = 0; i < 5; i++) {
                    position = func.find("|");
                    if (position == string::npos) {
                        tokens[i] = func;
                        break;
                    }
                    tokens[i] = func.substr(0, position);
                    func.erase(0, position + 1);
                }
                show_details(tokens);
            }
            reset(tokens);
        }
        details = !details;
    }

	return 0;
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
    if (!res)
        cout << "Failed to run executable";
    else
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
        << "\t-d, --debug executable compiled in DEBUG mode. retraive more information"
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
        else if (((arg == "-d") || (arg == "--debug"))) {
            debug = true;
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

void restoreAttribute() {
    SetConsoleTextAttribute(hConsole, _DEFAULT);
}

void colorful_output(const char* out, const int color) {
    SetConsoleTextAttribute(hConsole, color);
    cout << out;
    restoreAttribute();
}

void show_details(string args[5]) {
    cout << "\t" << "- function: "  << args[0] << endl;
    cout << "\t" << "- address: 0x" << args[1] << endl;
    cout << "\t" << "- location: "  << args[2] << endl;
    cout << "\t" << "- line: "      << args[3] << endl;
    cout << "\t" << "- module: "    << args[4] << endl;
    cout << "\n";
}

bool parse_headline(ifstream& ofile, const string& row, char (&addr)[9], int& sz, int& new_size, char (&new_addr)[9]) {
    if (row == "END")
        return true;
    if (row == "MALLOC") {
        ofile >> addr >> sz;
        colorful_output("[+]<malloc> ", _RED);
        printf("0x%s --> %d\n", addr, sz);
    }
    else if (row == "FREE") {
        ofile >> addr;
        colorful_output("[-]<free> ", _GREEN);
        printf("0x%s\n", addr);
    }
    else if (row == "REALLOC") {
        ofile >> addr >> new_size >> new_addr;
        colorful_output("[+]<realloc> ", _RED);
        printf("0x%s --> 0x%s, %d\n", addr, new_addr, new_size);
    }
    return false;
}
