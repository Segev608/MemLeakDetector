# MemLeakDetector
Detects memory leaks from Windows executables using the Microsoft Research Detours library.

## User Guide
After you have successfully downloaded the tool, there are several options available to you.

* -h, --help         Show this help message.

*  -o, --output      Specify the output path for a file that will contain all heap usage information.

*  -v, --verbose     Retraive warnings and more information about the leakes.

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
