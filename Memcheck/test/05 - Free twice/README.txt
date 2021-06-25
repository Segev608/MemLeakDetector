Until now, we have only seen leak problems.
What about general poor memory management?
The next few tests are talking exactly about this.

In this example you can see what happens when you free the same pointer twice.
This kind of error crashes your program.
Memcheck is able to stop the execution and notify you about this, with or without '-v'!
