#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

#define MAX_ARGS_COUNT 64
#define BUFFER_SIZE 1024 * 1024
#define MAX_PROCESS_COUNT 128
#define MAX_PATH_LENGTH 1024
#define CREDENTIAL_LENGTH 256

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"        // 
#define KGRN  "\x1B[32m"        //
#define KYEL  "\x1B[33m"        //  Color codes for the terminal
#define KBLU  "\x1B[34m"        //
#define KMAG  "\x1B[35m"        //
#define KCYN  "\x1B[36m"        //
#define KWHT  "\x1B[37m"

#include "queue.h"

static char output_buffer[BUFFER_SIZE];             // Buffer to hold the output of the block

static int is_last_block = 0;                       // Indicates whether the current block of input is the last block or not
static int is_first_block = 1;                      // Indicates whether the current block of input is the first block or not

static pid_t child_pids[MAX_PROCESS_COUNT];         // Holds the pids of the child processes 
static int child_pids_size = 0;

static char* current_working_directory;
static char* username;
static char* hostname;

static Queue history;                               // Holds the history of the commands

/**
 * Prints the history of the commands
 */
void executeHistory()
{   
    Node *temp = history.front;
    int index = 0;
    while (temp != NULL) {
        printf(" %4d  %s\n", ++index, temp->data);
        temp = temp->next;
    }
}

/**
 * Creates a child process and returns its process ID (pid)
 * @return The process ID (pid) of the child process
 */
pid_t createChildProcess()
{
    // check if the max process count is exceeded
    if(child_pids_size >= MAX_PROCESS_COUNT){
        fprintf(stderr, "Max process count exceeded\n");
        _exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        _exit(EXIT_FAILURE);
    }
    // add the pid to the child_pids array
    child_pids[child_pids_size++] = pid;
    return pid;
}

/**
 * Waits for all the child processes to finish
 */
void waitForAllChildProcesses()
{
    // loop through the child_pids array and wait for each child process
    for(int i = 0; i < child_pids_size; i++){
        while( waitpid(child_pids[i], NULL, 0) == -1){
            if(errno != EINTR){
                perror("waitpid");
                _exit(EXIT_FAILURE);
            }
        }
    }
    child_pids_size = 0;
}

/**
 * Resets the line info for the next line
 * Mainly used to reset the is_last_block and is_first_block flags
 */
void resetLineInfo()
{
    is_last_block = 0;
    is_first_block = 1;
}

/**
 * Trims the leading and trailing whitespaces of a string
 * @param str The string to be trimmed
 */
void trim(char* str)
{
    int i;
    int begin = 0;
    int end = strlen(str) - 1;

    while (isspace(str[begin]))
    {
        begin++;
    }

    while ((end >= begin) && isspace(str[end]))
    {
        end--;
    }

    for (i = begin; i <= end; i++)
    {
        str[i - begin] = str[i];
    }

    str[i - begin] = '\0';
}

/**
 * Parses a command string into an array of arguments
 * @param command The command string to be parsed
 * @return An array of arguments
 */
char** parseCommand(char *command)
{
    char **args = malloc(MAX_ARGS_COUNT * sizeof(char*));
    char *arg;
    int i = 0;

    arg = strtok(command, " ");
    while (arg != NULL)
    {
        args[i] = arg;
        i++;
        arg = strtok(NULL, " ");
    }
    args[i] = NULL;

    return args;
}

/**
 * Copies the contents of the buffer to the write end of the pipe
 * @param buffer The buffer containing the data to be copied
 * @param pipefd The file descriptors of the pipe used for communication
 */
void copyBufferToProcessInput(char *buffer, int pipefd[2]) 
{ 
    ssize_t bytes_written = write(pipefd[1], buffer, strlen(buffer));
    if (bytes_written == -1) {
        perror("write");
        _exit(EXIT_FAILURE);
    }
}

/**
 * Copies the output from read end of pipe to the output buffer
 * @param pipefd The file descriptors of the pipe used for communication
 */
void copyProcessOutputToBuffer(int pipefd[2]) {
    ssize_t bytes_read = read(pipefd[0], output_buffer, BUFFER_SIZE);
    if (bytes_read == -1) {
        perror("read");
        _exit(EXIT_FAILURE);
    }
    output_buffer[bytes_read] = '\0';
}

/**
 * Executes a command
 * @param command The command to be executed
 * @param from_child_pipefd The file descriptors of the pipe used for communication with the child process
 */
void executeCommand(char *command, int from_child_pipefd[2])
{
    trim(command); // Remove leading and trailing whitespaces
    if (strlen(command) == 0) // Return if the command is empty
    {
        return;
    }

    // Copy the command and add it to the history
    char* command_copy = (char*) malloc((strlen(command) + 1) * sizeof(char));
    strcpy(command_copy, command);
    enqueue(&history, command_copy);

    // Parse the command into arguments
    char** args = parseCommand(command);

    // Check if the command is a special command
    if(strcmp(command, "cd") == 0)
    {
        if(chdir(args[1]) == -1) // Change the current working directory
        {
            perror("chdir");
            _exit(EXIT_FAILURE);
        }
        getcwd(current_working_directory, MAX_PATH_LENGTH); // Update the current working directory
        return;
    }

    if(strcmp(command, "quit") == 0)
    {
        waitForAllChildProcesses(); // Wait for all child processes to finish
        exit(EXIT_SUCCESS);
    }

    //printf("Executing command: %s\n", command);
    pid_t pid;
    
    pid = createChildProcess();

    if(pid == 0) // Child process
    {
        if(!is_last_block)
        {
            // If the current block is not the last block, 
            // Redirect the output of the child process to the from_child_pipefd
            if(dup2(from_child_pipefd[1], STDOUT_FILENO) == -1)
            {
                perror("dup2");
                _exit(EXIT_FAILURE);
            }
        }

        if(!is_first_block)
        {
            // If the current block is not the first block,
            // Create a pipe and fill it with the output buffer
            // Redirect the from_child_pipefd to the input of the child process 
            int to_child_pipefd[2];
            if(pipe(to_child_pipefd) == -1)
            {
                perror("pipe");
                _exit(EXIT_FAILURE);
            }
            copyBufferToProcessInput(output_buffer, to_child_pipefd);

            if(dup2(to_child_pipefd[0], STDIN_FILENO) == -1)
            {
                perror("dup2");
                _exit(EXIT_FAILURE);
            }

            close(to_child_pipefd[0]);
            close(to_child_pipefd[1]);

        }

        if(strcmp(args[0], "history") == 0)
        {
            // execute the internal history command
            executeHistory();
            _exit(EXIT_SUCCESS);
        }

        // Execute the command
        execvp(args[0], args);
        perror("execvp");
        free(args);        
        _exit(EXIT_FAILURE);
    }
    else // Parent process
    {
        free(args);
    }

}

/**
 * Executes a line of commands
 * @param line The line of commands to be executed
 */
void executeLine(char *line)
{   
    int from_child_pipefd[2]; // File descriptors of the pipe used for communication 
                              // from the child process to the parent process
    if (pipe(from_child_pipefd) == -1 ) {
        perror("pipe");
        _exit(EXIT_FAILURE);
    }

    // Count the number of pipes in the line
    // to determine the number of blocks and related flags
    int pipe_count = 0;
    char *k = line; 
    while (*k != '\0') { 
        if (*k == '|') { pipe_count++; }
        k++; 
    }

    int seen_pipe_count = 0;
    char *c = line;
    while (*c != '\0')
    {
        // update flags   
        is_first_block = seen_pipe_count == 0;
        is_last_block = seen_pipe_count == pipe_count;
        
        if (*c == ';')
        {
            *c = '\0';
            executeCommand(line, from_child_pipefd);
            line = c + 1;
        }
        else if (*c == '|')
        {
            seen_pipe_count++;
            *c = '\0';
            executeCommand(line, from_child_pipefd);
            // wait for the child process to finish
            waitForAllChildProcesses();
            // copy the output of the child process to the buffer
            copyProcessOutputToBuffer(from_child_pipefd);
            line = c + 1;
        }
        c++;
    }

    executeCommand(line, from_child_pipefd);    // execute the last command
    waitForAllChildProcesses();                 // wait for the last child process to finish
    close(from_child_pipefd[0]);                // close the read end of the pipe
    close(from_child_pipefd[1]);                // close the write end of the pipe
    resetLineInfo();                            // reset the line info for the next line

}

/**
 * Enters the shell mode, where commands are entered interactively
 */
void shellMode()
{
    char line[251]; // line buffer
    char c;         // char to be read
    int cursor = 0; // cursor index to the buffer

    // Print the prompt
    printf("%s%s@%s%s:%s%s$ ",KGRN, username, hostname, KBLU, current_working_directory, KNRM);

    while(1)
    {
        if(cursor == 251)
        {
            printf("Error: Command too long\n");
            printf("Discarding the input\n");
            cursor = 0;
            continue;
        }
        c = getchar();

        if(c == EOF)
        {
            break;
        }

        if(c == '\n')
        {
            line[cursor] = '\0';

            executeLine(line);
            cursor = 0;
            printf("%s%s@%s%s:%s%s$ ",KGRN, username, hostname, KBLU, current_working_directory, KNRM);
            
            continue;
        }
        line[cursor++] = c;
    }
}

/**
 * Executes commands from a batch file
 * @param filename The name of the batch file
 */
void batchMode(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error: File not found\n");
        return;
    }

    char line[251];
    while (fgets(line, 251, file))
    {
        printf("Executing: %s\n",line);
        executeLine(line);
    }
    fclose(file);
}


int main( int argc, char *argv[] )
{
    // Initialize user info
    username = (char*)malloc(CREDENTIAL_LENGTH * sizeof(char));
    hostname = (char*)malloc(CREDENTIAL_LENGTH * sizeof(char));
    current_working_directory = (char* )malloc(MAX_PATH_LENGTH * sizeof(char));

    if (getlogin_r(username, CREDENTIAL_LENGTH) != 0)
    {
        perror("getlogin_r");
        exit(EXIT_FAILURE);
    }

    if (gethostname(hostname, CREDENTIAL_LENGTH) != 0)
    {
        perror("gethostname");
        exit(EXIT_FAILURE);
    }

    if (getcwd(current_working_directory, MAX_PATH_LENGTH) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    if (argc > 1)
    {
        // loop over the batch files if multiple are given as input
        for (int i = 1; i < argc; i++)
        {
            batchMode(argv[i]);
            waitForAllChildProcesses(); // wait so that seperate files do not run concurently
        }
    }
    else
    {
        shellMode();
    }

    // free the heap memory
    while(!isEmpty(&history))
    {
        free(dequeue(&history));
    }
    free(username);
    free(hostname);
    free(current_working_directory);

    return 0;
}
