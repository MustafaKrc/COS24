#ifndef freeList_h
#define freeList_h


#include <stdio.h>
#include <unistd.h>

#define MAGIC_NUMBER 0x12345678

typedef struct Node {
    size_t size;
    struct Node* next;
    int magicNumber;
    int isFreed;
} Node;

typedef struct FreeList {
    Node* head;
    Node* nextFreeBlock;
} FreeList;

typedef enum Strategy {
    BEST_FIT,
    WORST_FIT,
    FIRST_FIT,
    NEXT_FIT
} Strategy;

void* bestFit(FreeList* freeList, size_t size);
void* worstFit(FreeList* freeList, size_t size);
void* firstFit(FreeList* freeList, size_t size);
void* nextFit(FreeList* freeList, size_t size);

void split(Node* node, Node* prev, size_t size, FreeList* freeList);

Node* findPrevNode(Node* node, Node* head);

Node* coalesceFreeList(Node* freedBlock, Node* prev, Node* next);

#endif /* freeList_h */