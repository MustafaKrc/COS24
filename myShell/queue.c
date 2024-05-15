#include "queue.h"


Queue* createQueue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->front = queue->rear = NULL;
    return queue;
}

int isEmpty(Queue *queue) {
    return (queue->front == NULL);
}

void enqueue(Queue *queue, char* data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    
    if (isEmpty(queue)) {
        queue->front = newNode;
        queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
    //printf("Enqueued: %s\n", data);
}

char* dequeue(Queue *queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty!\n");
        return NULL;
    }
    char* data = queue->front->data;
    Node *temp = queue->front;
    queue->front = queue->front->next;
    free(temp);
    
    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    //printf("Dequeued: %s\n", data);
    
    return data;
}

void printQueue(Queue *queue) {
    Node *temp = queue->front;
    while (temp != NULL) {
        printf("%s\n", temp->data);
        temp = temp->next;
    }
}