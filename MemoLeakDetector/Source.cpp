#include <iostream>
#include <windows.h>
#include <fstream>
#include <detours.h>
using namespace std;

// options declerations
bool output_flag = false;
char* output = (char*)"diagnostic.txt";
char* exe = nullptr;
bool debug = false;

HANDLE run_exe();
LPTSTR get_exe();
bool is_x64(char*);
void show_usage(string);
void parse_args(int, char**);
void handle_output_path();

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
        cout << "ERROR";
        exit(0);
    }
    string func;
    
    char line[256] { 0 };
    while (strcmp(line, "START") != 0)
        ofile.getline(line, 256);

    while (ofile >> func)
    {
        if (func == "END")
            break;

        char addr[9];
        if (func == "MALLOC") {
            int sz;
            ofile >> addr >> sz;

            cout << func << ' ' << addr << ' ' << sz << '\n';
            //handle malloc
        }
        else if (func == "FREE") {
            ofile >> addr;
            cout << func << ' ' << addr << '\n';
            //handle free
        }
        else if (func == "REALLOC") {
            int new_size;
            char new_addr[9];
            ofile >> addr >> new_size >> new_addr;
            cout << func << ' ' << addr << ' ' << new_size << ' ' << new_addr << '\n';
            //handle realloc
        }
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
    if (argc < 2) {
        show_usage(argv[0]);
    }

    //parsing
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];

		if ((arg == "-h") || (arg == "--help")) {
			show_usage(argv[0]);
		}
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