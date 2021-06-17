# MemLeakDetector
Detects memory leaks from Windows executables using the Microsoft Research Detours library.

## Installation
As a user, you have two ways to use the system.

If you are working on the Visual Studio IDE and want to improve the system and contribute, feel free to do so by cloning our git repository on your system and running the solution file. Of course, it is highly recommended that files you want to test on the system will be compiled in Debug mode. This way you can enjoy the additional information that the system is able to obtain on executable files with debug mode compilation.

On the other hand, if you are interested in using the tool for the purpose of developing your projects, you have come to the right place!
Here is the recommended way to perform the installation:

1. Open the Memcheck folder which contains our tool
2. Prepare a C file that you wish to perform an memory analyze on
4. compile this code using those flags:  `cl /Zi /MDd /Od your-file.c` which should produce the following files:
  
![image](https://user-images.githubusercontent.com/57367786/122437588-83557700-cfa2-11eb-9add-6f63f1c3308e.png)

  
## User Guide
After you have successfully downloaded the tool, there are several options available to you.

* -h, --help         Show this help message.

* -o, --output      Specify the output path for a file that will contain all heap usage information.

* -v, --verbose     Retraive warnings and more information about the leakes.

In order to perform an analyzing on executable file:

<img src=https://user-images.githubusercontent.com/57367786/122337987-bc143280-cf47-11eb-8bb3-2c67ce919eab.png width="500" height="350" />

--verbose flag result:

--output flag results:

<img src=https://user-images.githubusercontent.com/57367786/122339574-f1218480-cf49-11eb-8430-fe81397570cc.png width="500" height="400" />

Explain:
1. Which memory function performed  
2. Pointer Address in the memory
3. Allocation amount (realloc function also prints the new pointer)
4. Stack trace 
5. Location address in the memory 
6. File path where the action happend 
7. The code line number  
8. Module name
