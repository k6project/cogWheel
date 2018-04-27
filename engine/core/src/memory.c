#include <core/memory.h>

void* memStackAllocStatic(memStackAlloc_t* stack, size_t size)
{
    void* result = NULL;
    if (!stack->lastMarker)
    {
        size = (size + 15u) & (~15u);
        if (size <= stack->freeBytes)
        {
            result = stack->memDynamic;
            stack->memDynamic += size;
            stack->freeBytes -= size;
        }
    }
    return result;
}

void* memStackAlloc(memStackAlloc_t* stack, size_t size, memStackMarker_t* marker)
{
    void* result = NULL;
    size += sizeof(memStackMarker_t);
    size = (size + 15u) & (~15u);
    if (size <= stack->freeBytes)
    {
        memStackMarker_t* marker = NULL;
        if (!stack->lastMarker)
        {
            marker = (memStackMarker_t*)stack->memDynamic;
        }
        else
        {
            marker = (memStackMarker_t*)((char*)stack->lastMarker + stack->lastMarker->size);
        }
        /*is POT: n and n-1 == 0*/
        /* decrement changes least significant non-zero bit to zero, and all bits to the right of it to 1 */
        result = ((char*)marker) + sizeof(memStackMarker_t);
        marker->prev = stack->lastMarker;
        stack->lastMarker = marker;
        stack->memDynamic += size;
        stack->freeBytes -= size;
    }
    return result;
}

void memStackFree(memStackMarker_t* marker)
{
    
}
