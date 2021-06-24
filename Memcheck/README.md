For the full experience, you can read this file from our GitHub repository:
https://github.com/Segev608/MemLeakDetector

# MemLeakDetector
Detects memory leaks from Windows executables using the Microsoft Research Detours library.

## Installation
As a user, you have two ways to use the system.

### Visual Studio
If you are working with Visual Studio IDE, you can clone this repository and run the `sln` file. The next step is to download Microsoft Research Detours library from nuget. You can do this by going to `Tools -> NuGet Package Manager -> Manage NuGet Packages for Solution` and in the 'Browse' section simply search "Detours". You need to download it at least for the "InjectedDLL" and "MemoLeakDetector" projects. Once you are finished it's done!

From here you can use `test.cpp` in "TestSamples" project to write you code, or write it and compile it yourself! Of course, it is highly recommended that files you want to test on the system will be compiled in Debug mode. This way you can enjoy the additional information that the system is able to obtain on executable files with debug mode compilation.

After you wrote compiled your code, you need to give your executable as an argument to our program. you can do this by right-click the start-up project ("MainApp") and from there `Properties -> Configuration Properties -> Debugging` and there you can change the command arguments and provide your executable. Don't forget to take into account the working directory when providing your executable's path! (Do NOT change it!). The full walkthough of the arguments and output is later in the "User Guide" section.

![image](https://user-images.githubusercontent.com/57449384/122484505-668a6500-cfdd-11eb-818c-69574af6333f.png)

You can now run the application like you normally would with Visual Studio.

### Independent
On the other hand, if you are more of a command-line person, you have come to the right place!
Here is the recommended way to perform the installation and use our tool:

1. Extract the Memcheck folder from our repository which contains our tool
2. Prepare a C file that you wish to perform a memory analyze on (or use the existing ones in 'test' folder)
3. run `vcvars32.bat` or `vcvars64.bat` to enter your prefered environment
4. Compile your code using `cl` with the following flags `/Zi /MDd /Od`: `cl /Zi /MDd /Od MyCFile.c` which should produce the following files:

(Explanation taken from the msdn website - https://docs.microsoft.com/)

`/Zi` - Produces a separate PDB file that contains all the symbolic debugging information for use with the debugger. The debugging information isn't included in the object files or executable, which makes them much smaller.

`/MDd` - Causes the application to use the debug multithread-specific and also causes the compiler to place the library name MSVCRTD.lib into the .obj file.

`/Od` - Turns off all optimizations in the program and speeds compilation.

![image](https://user-images.githubusercontent.com/57367786/122437588-83557700-cfa2-11eb-9add-6f63f1c3308e.png)

You are now ready to use memchek!
  
## User Guide
After you have successfully downloaded the tool, and compiled your code, you are ready to go!

Usage: `memcheck.exe <options> EXECUTABLE`

The executable is the path to your program. If the path contains spaces don't forget to wrap it with quotation marks!

there are several options available to you.

* -h, --help        Show a help message.

* -o, --output      Specify the output path for a file that will contain all heap usage information.

* -v, --verbose     Retraive warnings and more information about the leakes.

In order to perform an analyzing on executable file:

![image](https://user-images.githubusercontent.com/57449384/122476747-5f5c5a80-cfcf-11eb-842d-a28970412ef8.png)

In the upper section `Process Output` you can see your program's output.
In the lower section `Analysis` you can see the analysis of your program's memory managment.

There are 4 types of notifications:

* Warnings    - They do not involve a leak, and do not interfere the flow of the program, yet it's important for you to know.
* Errors      - They do not involve a leak, but it's a fatal error that makes your program crash. full trace provided.
* Loss Record - This is a specific leak accuring in your program. informs about the size of the leak and its trace.
* Summary     - This is the overall view of your program. how many bytes leaked ("in use at exit") and total heap usage.
  
In case you are not using `-v`, we won't show warnings or loss records, only errors and summary. 

--output flag results:

<img src=https://user-images.githubusercontent.com/57367786/122339574-f1218480-cf49-11eb-8430-fe81397570cc.png width="500" height="400" />

This is a detailed file about your program's heap usage. Every call to a memory managment function is recorded to this file,
including the arguments, returned value and full trace. your program's usage is between the `START` and `END` statments.
