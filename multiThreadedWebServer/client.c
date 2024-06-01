#include "blg312e.h"
#include <pthread.h>

// MAX_THREADS defines how many request will be done to the server in parallel
// The requested files are named as <filename prefix><number>.txt
// For example, if MAX_THREADS is 10, the client will request 10 files named as file1.txt, file2.txt, ..., file10.txt
#define MAX_THREADS 15

typedef struct {
  char *host;
  int port;
  char *filename;
} thread_args_t;

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "GET %s HTTP/1.1\r\n", filename);
  sprintf(buf, "%shost: %s\r\n\r\n", buf, hostname);
  Rio_writen(fd, buf, strlen(buf));
}
  
/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
  rio_t rio;
  char buf[MAXBUF];  
  int length = 0;
  int n;
  
  Rio_readinitb(&rio, fd);

  /* Read and display the HTTP Header */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (strcmp(buf, "\r\n") && (n > 0)) {
    printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);

    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
      printf("Length = %d\n", length);
    }
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (n > 0) {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
}

void* thread_func(void* arg) {
  thread_args_t *args = (thread_args_t*)arg;
  int clientfd;

  /* Open a single connection to the specified host and port */
  clientfd = Open_clientfd(args->host, args->port);

  clientSend(clientfd, args->filename);
  clientPrint(clientfd);

  Close(clientfd);

  free(args);
  pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
  if (argc < 5) {
    fprintf(stderr, "Usage: %s <host> <port> <filename prefixes> <file count>\n", argv[0]);
    exit(1);
  }

  char *host = argv[1];
  int port = atoi(argv[2]);
  int file_count = atoi(argv[4]);

  int num_files = file_count;
  if (num_files > MAX_THREADS) {
    fprintf(stderr, "Too many files. Max supported is %d\n", MAX_THREADS);
    exit(1);
  }

  pthread_t threads[MAX_THREADS];
  printf("Creating %d threads\n", num_files);
  for (int i = 0; i < num_files; i++) {
    thread_args_t *args = (thread_args_t*)malloc(sizeof(thread_args_t));
    args->host = host;
    args->port = port;
    
    // create file name (argv[3]+index)
    char* prefix = (char*)malloc(100*sizeof(char)); // +10 for the number at the end of the file name
    strcpy(prefix, argv[3]);
    char* postfix = (char*)malloc(10*sizeof(char));
    sprintf(postfix, "%d", i+1);
    strcat(prefix, postfix);
    strcat(prefix, ".txt");

    args->filename = prefix;

    free(postfix); // free postfix since it is not needed anymore

    printf("Creating thread for %s\n", args->filename);

    pthread_create(&threads[i], NULL, thread_func, args);
  }

  // join threads
  for (int i = 0; i < num_files; i++) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}

