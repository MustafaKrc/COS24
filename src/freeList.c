#include "../include/freeList.h"

/**
 * Finds the best fit block in the free list for a given size.
 * If a suitable block is found, it splits the block and returns a pointer to the allocated memory.
 * 
 * @param freeList The free list to search for the best fit block.
 * @param size The size of the block needed.
 * @return A pointer to the best fit block if found, NULL otherwise.
 */
void* bestFit(FreeList* freeList, size_t size)
{
    Node* current = freeList->head;
    Node* bestFit = NULL;
    Node* prev = NULL;
    Node* bestFitPrev = NULL;
    
    // iterate through the free list to find the best fit block
    while(current != NULL)
    {
        if(current->size >= size)
        {
            if(bestFit == NULL || current->size < bestFit->size)
            {
                bestFitPrev = prev;
                bestFit = current;
            }
        }
        if(current->next == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    // if a best fit block is found, split the block and return a pointer to the allocated memory
    if(bestFit != NULL)
    {
        split(bestFit, bestFitPrev, size, freeList);
        bestFit->magicNumber = MAGIC_NUMBER;
        bestFit->isFreed = 0;
        return (void*)((char*)bestFit + sizeof(Node));
    }

    return NULL;
}

/**
 * Finds the worst fit block in the free list that can accommodate the given size.
 * If a suitable block is found, it splits the block and returns a pointer to the allocated memory.
 * 
 * @param freeList The free list to search in.
 * @param size The size of the block to allocate.
 * @return A pointer to the allocated block, or NULL if no suitable block is found.
 */
void* worstFit(FreeList* freeList, size_t size)
{
    Node* current = freeList->head;
    Node* worstFit = NULL;
    Node* prev = NULL;
    Node* worstFitPrev = NULL;

    // iterate through the free list to find the worst fit block
    while(current != NULL)
    {
        if(current->size >= size)
        {
            if(worstFit == NULL || current->size > worstFit->size)
            {
                worstFit = current;
                worstFitPrev = prev;
            }
        }
        if(current->next == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    // if a worst fit block is found, split the block and return a pointer to the allocated memory
    if(worstFit != NULL)
    {
        split(worstFit, worstFitPrev, size, freeList);
        worstFit->magicNumber = MAGIC_NUMBER;
        worstFit->isFreed = 0;
        return (void*)((char*)worstFit + sizeof(Node));
    }

    return NULL;
}

/**
 * Finds the first block in the free list that is large enough to accommodate the requested size.
 * If a suitable block is found, it splits the block and returns a pointer to the allocated memory.
 * 
 * @param freeList The free list to search for a suitable block.
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory, or NULL if no suitable block is found.
 */
void* firstFit(FreeList* freeList, size_t size)
{
    Node* current = freeList->head;
    Node* prev = NULL;

    // iterate through the free list to find the first block that can accommodate the requested size
    while(current != NULL)
    {
        // if the current block is large enough, split the block and return a pointer to the allocated memory
        if(current->size >= size)
        {
            split(current, prev, size, freeList);
            current->magicNumber = MAGIC_NUMBER;
            current->isFreed = 0;
            return (void*)((char*)current + sizeof(Node));
        }
        if(current->next == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }
    return NULL;
}

/**
 * Finds the next available block in the free list using the next-fit algorithm.
 * If a suitable block is found, it splits the block and returns a pointer to the allocated memory.
 * 
 * @param freeList The free list to search in.
 * @param size The size of the block to allocate.
 * @return A pointer to the allocated block, or NULL if no suitable block is found.
 */
void* nextFit(FreeList* freeList, size_t size)
{
    Node* current = freeList->nextFreeBlock;
    Node* prev = findPrevNode(current, freeList->head);
    
    // iterate through the free list to find the next available block
    while(current != NULL)
    {
        // if the current block is large enough, split the block and return a pointer to the allocated memory
        if(current->size >= size)
        {
            split(current, prev, size, freeList);
            current->magicNumber = MAGIC_NUMBER;
            current->isFreed = 0;
            return (void*)((char*)current + sizeof(Node));
        }
        if(current->next == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    // if no suitable block is found, start from the beginning of the free list
    current = freeList->head;
    prev = NULL;
    
    // iterate through the free list to find the next available block until the next-fit block is reached
    while(current != freeList->nextFreeBlock)
    {
        // if the current block is large enough, split the block and return a pointer to the allocated memory
        if(current->size >= size)
        {
            split(current, prev, size, freeList);
            current->magicNumber = MAGIC_NUMBER;
            current->isFreed = 0;
            return (void*)((char*)current + sizeof(Node));
        }
        if(current->next == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    return NULL;
}

/**
 * Splits a node in the free list into two nodes, if there is enough space.
 * 
 * @param node The node to be split.
 * @param prev The previous node in the free list.
 * @param size The size of the memory block to be allocated.
 * @param freeList The free list structure.
 */
void split(Node* node, Node* prev, size_t size, FreeList* freeList)
{
    Node* newFreeNode = NULL;
    // if there are enough space to split
    if(node->size > size + sizeof(Node))
    {
        newFreeNode = (Node*)((char*)node + sizeof(Node) + size);
        newFreeNode->size = node->size - size - sizeof(Node);
        newFreeNode->next = node->next;
        node->size = size;
        //node->next = newFreeNode;
        if(prev != NULL)
        {
            prev->next = newFreeNode;
        }
        else
        {
            freeList->head = newFreeNode;
        }
    }
    else // if there are no enough space to split
    {
        // node size stays the same 
        // in other words, give all of free block to the allocated block
        if(prev != NULL)
        {
            prev->next = node->next;
        }
        else
        {
            freeList->head = node->next;
        }
        newFreeNode = node->next;
    }

    // if newFreeNode is null, it means that the last block is allocated
    // update the nextFreeBlock accordingly
    if(newFreeNode == NULL)
    {
        freeList->nextFreeBlock = freeList->head;
    }
    else
    {
        freeList->nextFreeBlock = newFreeNode;
    }

}

/**
 * Finds the previous node of a given node in a linked list.
 *
 * @param node The node whose previous node needs to be found.
 * @param head The head of the linked list.
 * @return The previous node of the given node, or NULL if the given node is not found or if it is the head node.
 */
Node* findPrevNode(Node* node, Node* head)
{
    Node* current = head;
    Node* prev = NULL;
    while(current != NULL)
    {
        if(current == node)
        {
            return prev;
        }
        if(current->next == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }
    return NULL;
}


/**
 * Coalesces adjacent free blocks in the free list.
 *
 * @param freedBlock The freed block to be coalesced with adjacent blocks.
 * @param prev The previous block in the free list.
 * @param next The next block in the free list.
 * @return A pointer to the merged block if merging is possible, otherwise NULL.
 */

Node* coalesceFreeList(Node* freedBlock, Node* prev, Node* next)
{

    // if freed block is before head
    if(prev == NULL)
    {
        // if freed block is right before head
        if((char*)freedBlock + freedBlock->size + sizeof(Node) == (char*)next)
        {
            freedBlock->size += next->size + sizeof(Node);
            freedBlock->next = next->next;
        }
        return freedBlock;
    }

    // if freed block is after head
    if(next == NULL)
    {
        if((char*)prev + prev->size + sizeof(Node) == (char*)freedBlock)
        {
            prev->size += freedBlock->size + sizeof(Node);
            prev->next = freedBlock->next;
        }
        return prev;
    }

    // freed block is between other blocks

    // if 3 blocks can be merged
    if(((char*)freedBlock + freedBlock->size + sizeof(Node) == (char*)next) && 
        ((char*)prev + prev->size + sizeof(Node) == (char*)freedBlock))
    {
        prev->size += freedBlock->size + next->size + 2 * sizeof(Node);
        prev->next = next->next;
        return prev;
    }
    // if prev and freed node can be merged
    else if((char*)freedBlock + freedBlock->size + sizeof(Node) == (char*)next)
    {
        freedBlock->size += next->size + sizeof(Node);
        freedBlock->next = next->next;
        return prev;
    }
    // if next and freed node can be merged
    else if((char*)prev + prev->size + sizeof(Node) == (char*)freedBlock)
    {
        prev->size += freedBlock->size + sizeof(Node);
        prev->next = freedBlock->next;
        return freedBlock;
    }

    return NULL;
}


