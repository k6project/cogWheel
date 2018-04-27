#include <core/memory.h>

void* memStackAllocStatic(memStackAlloc_t* stack, size_t size)
{
    void* result = NULL;
    if (!stack->lastMarker)
    {
        size = (size + 15u) & (~15u);
        stack->memDynamic += size;
    }
    return result;
}

void* memStackAlloc(memStackAlloc_t* stack, size_t size, memStackMarker_t* marker)
{
    void* result = NULL;
    size = (size + 15u) & (~15u);
    
    return result;
}

void memStackFree(memStackMarker_t* marker)
{
    
}
