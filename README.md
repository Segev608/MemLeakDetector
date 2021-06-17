# MemLeakDetector
Detects memory leaks from Windows executables using the Microsoft Research Detours library.

## Installation
As a user, you have two ways to use the system.

If you are working on the Visual Studio IDE and want to improve the system and contribute, feel free to do so by cloning our git repository on your system and running the solution file. Of course, it is highly recommended that files you want to test on the system will be compiled in Debug mode. This way you can enjoy the additional information that the system is able to obtain on executable files with debug mode compilation.

On the other hand, if you are interested in using the tool for the purpose of developing your projects, you have come to the right place!
Here is the recommended way to perform the installation:

1. Extract the Memcheck folder from our repository which contains our tool
2. Prepare a C file that you wish to perform an memory analyze on
3. run `vcvars32.bat` or `vcvars64.bat` to enter your prefered environment
4. compile your code using `cl` with the following flags `/Zi /MDd /Od`: `cl /Zi /MDd /Od MyCFile.c` which should produce the following files:
  
![image](https://user-images.githubusercontent.com/57367786/122437588-83557700-cfa2-11eb-9add-6f63f1c3308e.png)

  
## User Guide
After you have successfully downloaded the tool, and compiled your code, you are ready to go!
there are several options available to you.

* -h, --help        Show a help message.

* -o, --output      Specify the output path for a file that will contain all heap usage information.

* -v, --verbose     Retraive warnings and more information about the leakes.

In order to perform an analyzing on executable file:

![image](https://user-images.githubusercontent.com/57449384/122476747-5f5c5a80-cfcf-11eb-842d-a28970412ef8.png)

In the upper section `Process Output` you can see your program's output.
In the lower section `Analysis` you can see the analysis of you program's memory managment.

There are 4 types of notifications:

* Warnings    - They do not involve a leak, and do not interfere the flow of the program, yet it's important for you to know.
* Errors      - They do not involve a leak, but it's a fatal error that makes the program crash. full trace provided.
* Loss Record - This is a specific leak accuring in your program. informs about the size of the leak and its trace.
* Summary     - This is the overall view on your program. how many bytes leaked ("in use at exit") and total heap usage.
  
In case you are not using `-v`, we won't show warnings or loss records, only errors and summary. 

--output flag results:

<img src=https://user-images.githubusercontent.com/57367786/122339574-f1218480-cf49-11eb-8430-fe81397570cc.png width="500" height="400" />

This is a detailed file about your program's heap usage. Every call to a memory managment function is recorded to this file,
including the arguments, returned value and full trace. your program's usage is between the `START` and `END` statments.
