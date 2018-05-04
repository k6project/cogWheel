#include <core/memory.h>

#include <assert.h>

/*is POT: n and n-1 == 0*/
/* decrement changes least significant non-zero bit to zero, and all bits to the right of it to 1 */
#define IS_POW2(n) (!(n&(n-1)))

#if !IS_POW2(MEM_ALIGN_DEFAULT)
#error Default memory alignment should be power of two
#elif !MEM_ALIGN_DEFAULT
#error Default memory alignment should not be zero
#else
#define ALIGN_MASK ((size_t)(MEM_ALIGN_DEFAULT-1))
#endif

#define IS_ALIGNED(addr) (!(((size_t)addr) & ALIGN_MASK))

void* memStackAlloc(memStackAlloc_t* stack, size_t size)
{
    void* result = NULL;
    size += sizeof(memStackMarker_t);
    size = (size + ALIGN_MASK) & (~ALIGN_MASK);
    if (size <= stack->freeBytes)
    {
        memStackMarker_t* marker = NULL;
        if (!stack->lastMarker)
        {
            marker = (memStackMarker_t*)stack->memBase;
        }
        else
        {
            marker = (memStackMarker_t*)((char*)stack->lastMarker + stack->lastMarker->size);
        }
        result = ((char*)marker) + sizeof(memStackMarker_t);
        marker->prev = stack->lastMarker;
        stack->lastMarker = marker;
        stack->freeBytes -= size;
    }
    return result;
}

void memStackFree(memStackAlloc_t* stack, void* mem)
{
    /* todo: check for passing a pointer in the middle of allocation */
    char* addr = (mem) ? ((char*)mem) - sizeof(memStackMarker_t) : stack->memBase;
    assert(IS_ALIGNED(addr) && (addr > stack->memBase) && (addr < stack->memLast));
    memStackMarker_t* marker = (memStackMarker_t*)addr;
    stack->lastMarker = marker->prev;
    stack->freeBytes += marker->size;
}
