#ifndef heapAllactor_h
#define heapAllactor_h 

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "freeList.h"


#define PAGE_SIZE getpagesize()

size_t getHeapAddress(void* ptr);

int InitMyMalloc(int heapSize);
void* MyMalloc(size_t size, int strategy);
int MyFree(void* ptr);
void DumpFreeList();

void unInitMyMalloc();


#endif /* heapAllactor_h */