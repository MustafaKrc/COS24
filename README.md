# Computer Operating Systems Homeworks 24

## MyShell

This is different implementation of a command-line shell application. It has basic features such as running commands(processes) concurrently and handling piping operations. 

It differs from original shell with its use of reserved characters and how piping is handled. An example can be given as below:

shell>> command1 ; command2 | tee output.txt

In this example, command1 and command2 will run concurrently and their outputs will be piped to the tee command. But since these 2 commands are running concurrently, the output.txt file will be intertwined.

## SharedMalloc

This is a different implementation of malloc function as we know. This memory management system has ability to allocate space on a block to different processes with totally different address spaces. This allows processes to access data or "pointers" that are also accessable from other processes. This may cause race conditions, so user must be carefull in critical sections of code.

myMalloc and myFree functions are thread safe.


