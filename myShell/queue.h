#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>

typedef struct Node {
    char* data;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

Queue* createQueue();

int isEmpty(Queue *queue);

void enqueue(Queue *queue, char* data);

char* dequeue(Queue *queue);

void printQueue(Queue *queue);

// QUEUE_H
#endif 