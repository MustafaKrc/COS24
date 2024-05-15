#include "../include/heapAllocator.h"

static Node* heapBlockHeader = NULL;
static FreeList* p_sharedFreeList = NULL;
static pthread_mutex_t* mutex = NULL;
static size_t heapCapacity;


/**
 * Calculates the offset of a pointer from the start of the heap block.
 *
 * @param ptr The pointer whose offset needs to be calculated.
 * @return The offset of the pointer from the start of the heap block.
 */
size_t getHeapAddress(void* ptr)
{
    return (char*)ptr - (char*)heapBlockHeader;
}


/**
 * Initializes the memory allocator with a specified heap size.
 * 
 * @param heapSize The size of the heap in bytes.
 * @return 0 if initialization is successful, -1 otherwise.
 */
int InitMyMalloc(int heapSize)
{
    // check if heap is already initialized
    if(heapBlockHeader != NULL)
    {
        perror("Heap is already initialized");
        return -1;
    }

    // check if heap size is valid
    if(heapSize <= 0)
    {
        perror("Invalid heap size");
        return -1;
    }
    
    // check if heap size is a multiple of page size
    if(heapSize % PAGE_SIZE != 0)
    {
        // round up to the nearest page size
        heapSize = (1 + (heapSize / PAGE_SIZE)) * PAGE_SIZE;
    }

    // allocate heap
    void* heap = mmap(NULL, heapSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // check if allocation is successful
    if(heap == MAP_FAILED)
    {
        perror("Failed to allocate heap");
        return -1;
    }

    heapBlockHeader = (Node*)heap;
    heapBlockHeader->size = heapSize - sizeof(Node);
    heapBlockHeader->next = NULL;

    FreeList freeList;
    
    freeList.head = heapBlockHeader;
    freeList.nextFreeBlock = heapBlockHeader;

    heapCapacity = heapSize;

    // create a shared free list for child processes inside the heap
    // so that multiple processes can use the same memory block.
    p_sharedFreeList = (FreeList*)firstFit(&freeList, sizeof(FreeList));
    p_sharedFreeList->head = freeList.head;
    p_sharedFreeList->nextFreeBlock = freeList.nextFreeBlock;

    mutex = (pthread_mutex_t*)firstFit(p_sharedFreeList, sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);

    return 0;

}

/**
 * Allocates a block of memory of the specified size using the specified allocation strategy.
 *
 * @param size The size of the memory block to allocate.
 * @param strategy The allocation strategy to use (BEST_FIT, WORST_FIT, FIRST_FIT, NEXT_FIT).
 * @return A pointer to the allocated memory block, or NULL if the allocation fails.
 */
void* MyMalloc(size_t size, int strategy)
{
    // check if heap is initialized
    if(heapBlockHeader == NULL)
    {
        printf("Heap is not initialized\n");
        return NULL;
    }

    // mutex lock to prevent different processes from accessing the shared free list at the same time
    pthread_mutex_lock(mutex);
    void* returnPtr = NULL;
    switch(strategy)
    {
        case BEST_FIT:
            returnPtr = bestFit(p_sharedFreeList, size);
            break;
        case WORST_FIT:
            returnPtr = worstFit(p_sharedFreeList, size);
            break;
        case FIRST_FIT:
            returnPtr = firstFit(p_sharedFreeList, size);
            break;
        case NEXT_FIT:
            returnPtr = nextFit(p_sharedFreeList, size);
            break;
        default:
            returnPtr = NULL;
            break;
    }

    pthread_mutex_unlock(mutex);
    return returnPtr;
}

/**
 * Frees a previously allocated memory block.
 *
 * @param ptr Pointer to the memory block to be freed.
 * @return Returns 0 if the memory block is successfully freed, -1 otherwise.
 */

int MyFree(void* ptr)
{
    // check if heap is initialized
    if(heapBlockHeader == NULL)
    {
        perror("Heap is not initialized");
        return -1;
    }
    // mutex lock to prevent different processes from accessing the shared free list at the same time
    pthread_mutex_lock(mutex);
    
    if(ptr == NULL)
    {
        perror("Invalid pointer");
        pthread_mutex_unlock(mutex);
        return -1;
    }

    // get the block to be freed from the pointer
    Node* block = (Node*)((char*)ptr - sizeof(Node));
    

    // check boundries
    if((char*)block < (char*)heapBlockHeader || (char*)block > ((char*)heapBlockHeader + heapCapacity))
    {
        perror("Invalid pointer");
        pthread_mutex_unlock(mutex);
        return -1;
    }
    
    // check if block is a valid block
    if(block->magicNumber != MAGIC_NUMBER || block->isFreed == 1)
    {
        perror("Invalid pointer");
        pthread_mutex_unlock(mutex);
        return -1;
    }

    // block is valid
    block->isFreed = 1;

    // if freelist is empty, add block to freelist head
    if(p_sharedFreeList->head == NULL)
    {
        p_sharedFreeList->head = block;
        block->next = NULL;
        pthread_mutex_unlock(mutex);
        return 0;
    }

    // find previous free block before block
    Node* current = p_sharedFreeList->head;
    Node* previous = NULL;

    while(current != NULL)
    {
        if((char*) current > (char*)block)
        {
            break;
        }
        if(current->next == NULL)
        {
            break;
        }
        previous = current;
        current = current->next;
    }
    

    // freed block is before the first free block in free list
    if(previous == NULL)
    {
        block->next = p_sharedFreeList->head;
        p_sharedFreeList->head = block;
        Node* coalescedBlock = coalesceFreeList(block, previous, current);
        // if current was the nextFreeBlock, and current is merged with the freed block
        if(current == p_sharedFreeList->nextFreeBlock && coalescedBlock->next != current)
        {
            p_sharedFreeList->nextFreeBlock = coalescedBlock;
        }
        pthread_mutex_unlock(mutex);
        return 0;
    }

    // freed block is after the last free block in free list
    if(current == NULL)
    {
        previous->next = block;
        block->next = NULL;
        coalesceFreeList(block, previous, current);
        // no need to check for nextFreeBlock, since it will stay the same regardless.
        // because freed block is after the block that it has merged with
        pthread_mutex_unlock(mutex);
        return 0;
    }

    // freed block is between two free blocks
    previous->next = block;
    block->next = current;
    Node* coalescedBlock = coalesceFreeList(block, previous, current);

    // if current was the nextFreeBlock, and current is merged with the freed block
    if(current == p_sharedFreeList->nextFreeBlock && coalescedBlock->next != current)
    {
        p_sharedFreeList->nextFreeBlock = coalescedBlock;
    }
    pthread_mutex_unlock(mutex);
    return 0;

}

/**
 * Prints the information about the free blocks in the heap.
 * The function iterates through the free list and prints the address, size, and status (Free or Full) of each block.
 * If the free list is empty, it prints the address, size, and status of the entire heap.
 */
void DumpFreeList()
{
    if(heapBlockHeader == NULL)
    {
        perror("Heap is not initialized");
        return;
    }
    pthread_mutex_lock(mutex);
    Node* current = p_sharedFreeList->head;
    char* offset = (char*)heapBlockHeader;

    // if all off memory is occupied
    if(p_sharedFreeList->head == NULL)
    {
        printf("Address: %d, Size: %ld, Status: Full\n", 0, heapCapacity);
        pthread_mutex_unlock(mutex);
        return;
    }

    // if there is a used space before the first free block
    if((char*)p_sharedFreeList->head > (char*)heapBlockHeader)
    {
        printf("Address: %d, Size: %ld, Status: Full\n", 0, (char*)p_sharedFreeList->head - offset);
    }

    // iterate through the free list and print the address, size, and status of each block
    while(current->next != NULL)
    {       
        printf("Address: %ld, Size: %ld, Status: Free\n", (char*)current - offset, current->size + sizeof(Node));
        printf("Address: %ld, Size: %ld, Status: Full\n", (char*)current + current->size + sizeof(Node) - offset,
                                                          (char*)current->next - ((char*)current + current->size + sizeof(Node)));
        current = current->next;
    }
    // print the last free block
    printf("Address: %ld, Size: %ld, Status: Free\n", (char*)current - offset, current->size + sizeof(Node));
    
    // if there is a used space after the last free block
    if((char*)current + current->size + sizeof(Node) < (char*)heapBlockHeader + heapCapacity)
    {
        printf("Address: %ld, Size: %ld, Status: Full\n", (char*)current + current->size - offset + sizeof(Node),
                                                         heapCapacity - ((char*)current - offset + current->size + sizeof(Node)));
    }
    pthread_mutex_unlock(mutex);
}


void unInitMyMalloc()
{
    // check if heap is initialized
    if(heapBlockHeader == NULL)
    {
        perror("Heap is not initialized");
        return;
    }

    pthread_mutex_destroy(mutex);
    munmap(heapBlockHeader, heapCapacity);
    heapBlockHeader = NULL;
    p_sharedFreeList = NULL;
    heapCapacity = 0;
    mutex = NULL;
}

