#include <shapes/shapes.h>
#include <shapes/logging.h>
#include <shapes/memoryalloc.h>
#include <stdlib.h>

void alloc_memory(void **outPtr, int numElems, int elemSize)
{
        int numBytes = numElems * elemSize; /*XXX overflow*/
        void *ptr = malloc(numBytes);
        if (!ptr)
                fatal("OOM!\n");
        *outPtr = ptr;
}

void realloc_memory(void **inoutPtr, int numElems, int elemSize)
{
        int numBytes = numElems * elemSize; /*XXX overflow*/
        void *ptr = realloc(*inoutPtr, numBytes);
        if (!ptr)
                fatal("OOM!\n");
        *inoutPtr = ptr;
}

void free_memory(void **inoutPtr)
{
        free(*inoutPtr);
        *inoutPtr = NULL;
}
