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
void restoreAttribute();
void colorful_output(const char* out, const int color);

// options declerations
bool output_flag = false;
char* output = (char*)"C:\\diagnostic.txt";
char* exe = nullptr;
bool debug = false;

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

void anal(ifstream& ofile);
vector<string> split(string s, char t);
HANDLE run_exe();
LPTSTR get_exe();
bool is_x64(char*);
void show_usage(string, bool is_abort = false);
void parse_args(int, char**);
void handle_output_path();
void show_details(vector<string> args);
void new_call(vector<Call>& calls, const Call& call);
template<class T, class Pred>
int erase_if(vector<T>& v, Pred pred);

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
    anal(ofile);
   
    return 0;
}

void anal(ifstream& ofile) {
    char line[256]{ 0 };
    while (strcmp(line, "START") != 0)
        ofile.getline(line, 256);

    string func;
    char addr[9];
    int sz;
    char old_addr[9];
    int new_size;

    vector<Call> calls;

    while (ofile >> func)
    {
        if (func == "END")
            break;
        Call c;
        c.method = func.c_str();

        if (func == "MALLOC") {
            ofile >> addr >> sz;
            c._size = sz;
        }
        else if (func == "FREE") {
            ofile >> addr;
        }
        else if (func == "REALLOC") {
            ofile >> old_addr >> new_size >> addr;
            c.last_address = old_addr;
            c._size = new_size;
        }
        c.address = addr;

        char buf[1000]{ 0 };
        ofile.get();
        ofile.getline(buf, 1000);

        if (!buf[0])
            continue;

        auto result = split(string(buf), '|');

        c.caller = result[0];
        c.caller_addr = result[1];
        c.file = result[2] == "NULL" ? "" : result[2];
        c.caller_line = atoi(result[3].c_str());
        c._module = result[4] == "NULL" ? "" : result[4];

        
        //anal
        new_call(calls, c);
    }
    
    int lost_bytes = 0;
    for (int i = 0; i < calls.size(); ++i) {
        lost_bytes += calls[i]._size;
        printf("%d bytes are lost in loss record %d of %d\n", calls[i]._size, i + 1, calls.size());
        const char* file_name = strrchr(calls[i].file.c_str(), '\\');
        printf("\tat 0x%s: %s (%s:%d)\n", calls[i].caller_addr.c_str(), calls[i].caller.c_str(), file_name + 1, calls[i].caller_line);
    }

    cout << "\nHEAP SUMMARY:\n";
    printf("\tin use at exit: %d bytes\n", lost_bytes);
    printf("\ttotal heap usage: %d allocs, %d frees, %d reallocs.\n", total_allocs, total_frees, total_reallocs);
    printf("\t%d bytes allocated\n", total_bytes);
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
        else {
            total_allocs++;
            total_bytes += call._size;
            calls.push_back(call);
        }
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
            total_frees++;
        }
    }
    else if (call.method == "REALLOC") {
        int del = erase_if(calls, 
            [call](Call c) { return c.address == call.last_address; });
        if (del == 0) {
            cout << "reallocated non-existing pointer\n";
            // problem 
        }
        else {
            calls.push_back(call);
            total_reallocs++;
            total_bytes += call._size;
        }
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

void show_usage(std::string name, bool is_abort)
{
    std::cerr << "Usage: " << name << " <options> EXECUTABLE\n"
        << "Options:\n"
        << "\t-h, --help\t\tShow this help message\n"
        << "\t-o, --output OUTPUT\tSpecify the output path\n"
        << "\t-d, --debug executable compiled in DEBUG mode. retraive more information"
        << std::endl;
    if (is_abort)
        exit(0);
}

void parse_args(int argc, char** argv)
{
    //if (argc < 2) 
    //    show_usage(argv[0]);

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

void show_details(vector<string> args) {
    cout << "\t" << "- function: "  << args[0] << endl;
    cout << "\t" << "- address: 0x" << args[1] << endl;
    cout << "\t" << "- location: "  << args[2] << endl;
    cout << "\t" << "- line: "      << args[3] << endl;
    cout << "\t" << "- module: "    << args[4] << endl;
    cout << "\n";
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

void print_results() {

}
