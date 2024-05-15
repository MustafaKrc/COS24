#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "../include/heapAllocator.h"


void singleStrategyTest(Strategy strategy)
{
    void* ptr = MyMalloc(10, strategy);
    printf("ptr: %p\n", ptr);
    DumpFreeList();
    printf("\n");
    
    void* ptr2 = MyMalloc(10, strategy);
    printf("ptr2: %p\n", ptr2);
    DumpFreeList();
    printf("\n");

    void* ptr3 = MyMalloc(10, strategy);
    printf("ptr3: %p\n", ptr3);
    DumpFreeList();
    printf("\n");

    MyFree(ptr2);
    printf("Freed ptr2\n");
    DumpFreeList();
    printf("\n");

    void* ptr4 = MyMalloc(10, strategy);
    printf("ptr4: %p\n", ptr4);
    DumpFreeList();
    printf("\n");

    printf("Freeing ptr\n");
    MyFree(ptr);
    DumpFreeList();
    printf("\n");

    printf("Freeing ptr3\n");
    MyFree(ptr3);
    DumpFreeList();
    printf("\n");

    printf("Freeing ptr4\n");
    MyFree(ptr4);
    DumpFreeList();
    printf("\n");

}

void testStrategies()
{
    printf("Testing FIRST_FIT strategy\n--------------------------------------------------------------\n");    
    singleStrategyTest(FIRST_FIT);
    printf("Testing NEXT_FIT strategy\n--------------------------------------------------------------\n");
    singleStrategyTest(NEXT_FIT);
    printf("Testing BEST_FIT strategy\n--------------------------------------------------------------\n");
    singleStrategyTest(BEST_FIT);
    printf("Testing WORST_FIT strategy\n--------------------------------------------------------------\n");
    singleStrategyTest(WORST_FIT);
}

void forkTest()
{
    int strategies[4];
    int requestSizes[4];
    printf("Malloc strategies\n");
    printf("0: BEST_FIT\n");
    printf("1: WORST_FIT\n");
    printf("2: FIRST_FIT\n");
    printf("3: NEXT_FIT\n");
    for(int i = 0; i<4; i++)
    {
        printf("Enter the fork number %d 's malloc strategy\n", i);
        scanf("%d", &strategies[i]);
        if(strategies[i] < 0 || strategies[i] > 3)
        {
            printf("Invalid strategy, enter again!\n");
            i--;
        }

        printf("Enter the fork number %d 's malloc size\n", i);
        scanf("%d", &requestSizes[i]);
    }

    Strategy currStrategy;
    int allocationSize;
    int pid;

    char* strategyNames[4] = {"BEST_FIT", "WORST_FIT", "FIRST_FIT", "NEXT_FIT"};

    printf("Heap before other processes\n");
    DumpFreeList();
    printf("\n");

    char** pointers = (char**)MyMalloc(4*sizeof(char*), FIRST_FIT);

    printf("Allocating memory from different processes in same memory block\n");
    for(int i = 0; i<4; i++)
    {
        currStrategy = strategies[i];
        allocationSize = requestSizes[i];
        pid = fork();
        if(pid == -1)
        {
            printf("Fork failed\n");
            _exit(EXIT_FAILURE);
        }
        if(pid == 0)
        {
            printf("P%d Child process\nChild PID: %d\nChild strategy: %s\n", i+1, getpid(), strategyNames[currStrategy]);   
            char* test = (char*)MyMalloc(allocationSize, currStrategy);
            
            if(test == NULL)
            {
                printf("Malloc failed in process PID %d for request size of %d\n\n", getpid(), allocationSize);
                _exit(EXIT_FAILURE);
            }

            printf("Malloc succeded in process PID %d with a pointer %d byte at address %ld in heap.\n\n", getpid(), allocationSize, getHeapAddress(test));
            pointers[i] = test;
            _exit(EXIT_SUCCESS);
        }

    }

    int status;
    pid_t wpid;

    // wait for childs to finish
    while((wpid = wait(&status)) > 0);

    DumpFreeList();

    printf("Freeing Pointers\n");
    for(int i = 0; i<4; i++)
    {
        MyFree(pointers[i]);
    }
    MyFree(pointers);

    DumpFreeList();
}

void utilizationTest()
{
    DumpFreeList();
    printf("\n");

    for(int i = 1; i<3; i++)
    {
        for(int j = -1; j<2; j++)
        {
            int usable_size = 3992; // hardcoded to be tested in 4096 kb
            int header_size = sizeof(Node);
            int request_size = usable_size - i*header_size + j;
            void* ptr = MyMalloc(request_size, FIRST_FIT);
            
            printf("Requested %d bytes, returned pointer: %p with size %d\n", request_size, ptr, ptr == NULL ? 0 : request_size);
            DumpFreeList();

            printf("Freeing ptr\n");
            MyFree(ptr);
            DumpFreeList();
            printf("\n");
        }
    }
}

void invalidPointerTest()
{
    void* ptr = MyMalloc(10, FIRST_FIT);
    printf("ptr: %p\n", ptr);
    DumpFreeList();
    printf("\n");

    printf("Freeing ptr\n");
    int result = MyFree(ptr);
    printf("MyFree result: %d\n", result);
    DumpFreeList();
    printf("\n");

    printf("Freeing ptr again\n");
    result = MyFree(ptr);
    printf("MyFree result: %d\n", result);
    DumpFreeList();
    printf("\n");

    printf("Freeing unvalid pointer\n");
    result = MyFree((void*)ptr + 10);
    printf("MyFree result: %d\n", result);
    DumpFreeList();
    printf("\n");

}

void conductTests()
{
    printf("Initializing MyMalloc\n");
    int mallocInitResult = InitMyMalloc(4096);
    printf("InitMyMalloc result: %d\n", mallocInitResult);

    printf("\nTesting strategies alone\n");
    testStrategies();
    printf("\nTesting full utilization\n");
    utilizationTest();
    printf("\nTesting shared Memory usage between processes\n");
    forkTest();

    printf("\nTesting freeing invalid pointer\n");
    invalidPointerTest();

    printf("\nUninitializing MyMalloc\n");
    unInitMyMalloc();

    void* ptr = MyMalloc(1, FIRST_FIT);
    if(ptr == NULL)
    {
        printf("ptr is NULL, MyMalloc has failed as expected (unitialized heap)\n");
    }
    else
    {
        printf("ptr: %p\n", ptr);
    }

}

int main() {

    conductTests();

    // if you want to use the heap allocator in your own program,
    // you must initialize it first using InitMyMalloc(size) function

    return 0;
}