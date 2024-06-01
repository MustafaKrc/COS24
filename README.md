# Computer Operating Systems Homeworks 24

## MyShell

This is different implementation of a command-line shell application. It has basic features such as running commands(processes) concurrently and handling piping operations. 

It differs from original shell with its use of reserved characters and how piping is handled. An example can be given as below:

shell>> command1 ; command2 | tee output.txt

In this example, command1 and command2 will run concurrently and their outputs will be piped to the tee command. But since these 2 commands are running concurrently, the output.txt file will be intertwined.

## SharedMalloc

This is a different implementation of malloc function as we know. This memory management system has ability to allocate space on a block to different processes with totally different address spaces. This allows processes to access data or "pointers" that are also accessable from other processes. This may cause race conditions, so user must be carefull in critical sections of code.

myMalloc and myFree functions are thread safe.

## Multithreaded Web Server

This is an implementation of a multithreaded web server capable of handling multiple client requests concurrently. It supports basic HTTP functionalities, serving static files from a specified directory as well as dynamic content.

The multithreaded web server utilizes a thread pool to manage incoming requests. The scheduling policy employed is crucial for the server's performance and responsiveness.

- **First-in, First-out (FIFO)**: Requests are handled in the order they are received. This straightforward approach ensures fairness, as each request is processed in sequence without priority, thus preventing starvation.

- **Smallest File First (SFF)**: The server prioritizes requests for smaller files over larger ones. This can reduce the average waiting time for requests and improve overall responsiveness, especially in environments where file sizes vary significantly.

- **Recent File First (RFF)**: Gives priority to requests for files that have been most recently modified. This can be particularly useful in scenarios where the most recent data is the most relevant, ensuring that users receive the latest content promptly.

Start the server with the following command:

> ./server \<port\> \<thread_pool_size\> \<buffer_size\> \<schedule_policy\>   

For example:

> ./server 8080 4 16 FIFO

This will start the server on port 8080 with a pool of 4 worker threads and a buffer of size 16.

A client program is provided to test the server. The client sends multiple HTTP GET requests to the server in parallel using pthreads.

Example command to run the client:

> ./client localhost 8080 file 15

This command connects to the server on `localhost` at port `8080` and requests 15 files named `file1.txt`, `file2.txt`, ..., `file15.txt`.

The server can also handle dynamic content. When a request for a dynamic resource is received, the server processes the request to generate the appropriate dynamic content before sending the response.

Ensure the server is running before starting the client program. The server and client programs should be executed on the same machine or network. The directory containing the static files to be served should be specified and accessible to the server.

