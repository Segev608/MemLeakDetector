Until now we compiled everything using specific flags, but its not always the case.
Now, try compiling your program using only the '/MD' flag.
Note that this flag has NOTHING to do with debug information.
Without it, there is a very high chance the the compiler will decide to replace 'malloc' with 'HeapAlloc' from the Windows API which we did not (yet) hooked.
It is there only to make sure malloc is being imported.

You can see in the results how the trace doesn't really tell anything because of all the optimizations
and the lack of important symbols.

Note: This test was compiled in x64. (Just for fun)
