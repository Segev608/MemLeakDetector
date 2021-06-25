What about if you want to test your DLL? Memcheck got your back!
The executable and the DLL were compiled with the same flags, but to compile into a dll you need to add the '/LD' flag.
There are 3 examples in this test:

1) no-info-32 - this test was compiled only with the '/MD' flag in x32.
2) with-info-32 - this test was compiled with all of the flags in x32.
3) with-info-64 - this test was compiled with all of the flags in x64.
