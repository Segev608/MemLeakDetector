Many programmers follow the freeing of a pointer with assigning NULL to it.
But what happens if you still free it after that?
Your program does not crash, but you should be aware of that.
Memcheck is able to detect this and warn you about this!
Note that to see warnings, you need to use '-v' option.
