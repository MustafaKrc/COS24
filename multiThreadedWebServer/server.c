#include "blg312e.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>

sem_t empty, fill, mutex;
int head, tail;
request_t *queue;
int nbuffer;
int count;
char *sched_policy;  // Scheduling policy

/**
 * Prints the elements in the queue along with the head and tail indices.
 */
void printQueue() {
    for(int i = 0; i < nbuffer; i++){
        printf("%d ", queue[i].connfd);
    }
    printf("head: %d, tail: %d\n", head, tail);
}

/**
 * Parses command line arguments and assigns values to variables.
 *
 * @param port      Pointer to the variable to store the port number.
 * @param nthreads  Pointer to the variable to store the number of threads.
 * @param argc      The number of command line arguments.
 * @param argv      Array of command line arguments.
 */
void getargs(int *port, int *nthreads, int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <port> <threads> <buffers> <sched_policy>\n", argv[0]);
        exit(1);
    }
    if((*port = atoi(argv[1])) <= 2000) {
      fprintf(stderr, "Port number must be larger than 2000");
      exit(1);
    }
    if((*nthreads = atoi(argv[2])) <= 0){
      fprintf(stderr, "Threads must be a positive integer");
      exit(1);
    }
    if((nbuffer = atoi(argv[3])) <= 0){
      fprintf(stderr, "Buffers must be a positive integer");
      exit(1);
    }
    if(strcmp(argv[4], "FIFO") != 0 && strcmp(argv[4], "SFF") != 0 && strcmp(argv[4], "RFF") != 0){
        fprintf(stderr, "Invalid scheduling policy\n");
        fprintf(stderr, "Available scheduling policies: FIFO, SFF, RFF");
        exit(1);
    }
    sched_policy = argv[4];

}

/**
 * Shifts the elements in the queue to the left starting from the specified index.
 * 
 * @param start_index The index from which to start shifting the elements.
 */
void shift_queue_left(int start_index) {
    // Shift the elements to the left
    for (int i = start_index; i != (tail - 1 + nbuffer) % nbuffer; i = (i + 1) % nbuffer) {
        int next_index = (i + 1) % nbuffer;
        queue[i] = queue[next_index];
    }
    // make last element empty
    queue[(tail - 1 + nbuffer) % nbuffer].connfd = -1;

    // Update the head and tail pointers
    if((tail - 1 + nbuffer) % nbuffer == head)
    {
        head = (head + 1) % nbuffer;
    }
    tail = (tail - 1 + nbuffer) % nbuffer;
}


/**
 * @brief This function is the entry point for a thread that handles incoming requests.
 * 
 * @param port The port number on which the server is listening.
 * @return void* Returns NULL.
 */
void* thread_handle(void* port) {
    while(1) {
        // wait for a request to be put into the queue
        sem_wait(&fill);
        sem_wait(&mutex);

        // find the target index depending on the scheduling policy
        int target = -1;
        if (strcmp(sched_policy, "FIFO") == 0) {
            target = head;
        } else if (strcmp(sched_policy, "RFF") == 0) {
            time_t latest = -1;
            // find the latest modified file
            for (int i = 0; i < nbuffer; i++) {
                if (queue[i].connfd != -1 && queue[i].sbuf.st_mtime > latest) {
                    latest = queue[i].sbuf.st_mtime;
                    //printf("latest: %ld\n",  ((queue[i].sbuf.st_mtime)));
                    target = i;
                }
            }
        } else if (strcmp(sched_policy, "SFF") == 0) {
            off_t smallest = LONG_MAX;
            // find the smallest file
            for (int i = 0; i < nbuffer; i++) {
                if (queue[i].connfd != -1 && queue[i].sbuf.st_size < smallest) {
                    smallest = queue[i].sbuf.st_size;
                    target = i;
                }
            }
        }

        // get the connection file descriptor
        int connfd = queue[target].connfd;
        // copy the request to a local variable
        request_t request;
        request.connfd = connfd;
        request.is_static = queue[target].is_static;
        request.stat_return = queue[target].stat_return;
        request.sbuf = queue[target].sbuf;
        strcpy(request.buf, queue[target].buf);
        strcpy(request.method, queue[target].method);
        strcpy(request.uri, queue[target].uri);
        strcpy(request.version, queue[target].version);
        strcpy(request.filename, queue[target].filename);
        strcpy(request.cgiargs, queue[target].cgiargs);
        request.rio = queue[target].rio;
        
        // make the slot empty in the queue
        queue[target].connfd = -1;

        // update the head if necessary
        if (target == head) {
            head = (head + 1) % nbuffer;
        } else { 
            // shift the elements after the target to the left if target is not head
            shift_queue_left(target);
        }
        // update the count
        count--;
        
        // signal the empty semaphore
        sem_post(&mutex);
        sem_post(&empty);

        requestHandle(connfd, request); // handle the request
        Close(connfd); // close the connection file descriptor
    }
}

/**
 * Puts a connection file descriptor into the queue.
 * Also reads the request line and headers from the connection file descriptor.
 * 
 * @param connfd The connection file descriptor to be put into the queue.
 */
void queue_put(int connfd) {

    sem_wait(&empty);
    sem_wait(&mutex);

    // initialize rio
    Rio_readinitb(&(queue[tail].rio), connfd);
    Rio_readlineb(&(queue[tail].rio), queue[tail].buf, MAXLINE);
    // read method, uri, version
    sscanf(queue[tail].buf, "%s %s %s", queue[tail].method, queue[tail].uri, queue[tail].version);

    // parse uri
    queue[tail].is_static = requestParseURI(queue[tail].uri, queue[tail].filename, queue[tail].cgiargs);
    // get file stats
    queue[tail].stat_return = stat(queue[tail].filename, &(queue[tail].sbuf));
    // read headers
    requestReadhdrs(&(queue[tail].rio));

    // update connfd
    queue[tail].connfd = connfd;
    // update queue
    tail = (tail + 1) % nbuffer;
    count++;

    sem_post(&mutex);
    sem_post(&fill);
}

int main(int argc, char *argv[])
{
    int listenfd, port, clientlen, nthreads, connfd;
    struct sockaddr_in clientaddr;
    getargs(&port, &nthreads, argc, argv);
    pthread_t *tids = (pthread_t*)malloc(sizeof(pthread_t) * nthreads);
    count = 0;
    head = 0;
    tail = 0;
    queue = (request_t*)malloc(sizeof(request_t) * nbuffer);
    for (int i = 0; i < nbuffer; i++) {
        queue[i].connfd = -1;  // Initialize empty slots
    }
    listenfd = Open_listenfd(port);

    sem_init(&mutex, 0, 1);  // semaphore for mutual exclusion
    sem_init(&empty, 0, nbuffer);  // semaphore for empty slots
    sem_init(&fill, 0, 0);  // semaphore for filled slots

    // create thread pool for handling requests
    for(int i = 0; i < nthreads; i++){
        pthread_create(&tids[i], NULL, thread_handle, NULL);
    }

    // start listening the port
    while (1) {    
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        printf("Client %d\n", connfd);
        queue_put(connfd);
    }

    // cleanup
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&fill);
    free(queue);
    free(tids);
}
