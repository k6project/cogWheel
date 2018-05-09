#include <core/memory.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

#define BIT_SET(addr,b) do {*(addr+(b>>3u)) |= (1u<<(b&7u));} while(0)
#define BIT_UNSET(addr,b) do {*(addr+(b>>3u)) &= ~(1u<<(b&7u));} while(0)
#define BIT_TEST(addr,b) ((*(addr+(b>>3u)) & (1u<<(b&7u))) == (1u<<(b&7u)))

#define IS_ALIGNED(addr) (!(((size_t)addr) & ALIGN_MASK))

struct memStackMarker_t
{
    struct memStackMarker_t* prev;
    size_t size;
} ALIGNED(MEM_ALIGN_DEFAULT);
typedef struct memStackMarker_t memStackMarker_t;

struct memStackAlloc_t
{
    char* memLast;
    struct memStackMarker_t* lastMarker;
} ALIGNED(MEM_ALIGN_DEFAULT);

void memStackInit(memStackAlloc_t** outStack, size_t size)
{
    memStackDestroy(outStack);
    size_t allocSize = (size + ALIGN_MASK) & (~ALIGN_MASK);
    size_t maskSize = (allocSize / sizeof(memStackAlloc_t)) >> 3;
    char* memory = (char*)calloc(1, allocSize + maskSize + sizeof(memStackAlloc_t));
    memStackAlloc_t* stack = (memStackAlloc_t*)memory;
    stack->memLast = memory + sizeof(memStackAlloc_t) + allocSize;
    *outStack = stack;
}

void* memStackAlloc(memStackAlloc_t* stack, size_t size)
{
    void* result = NULL;
    char* base = ((char*)stack) + sizeof(memStackAlloc_t);
    size = (size + ALIGN_MASK) & (~ALIGN_MASK);
    size += sizeof(memStackMarker_t);
    memStackMarker_t* last = stack->lastMarker;
    char* markerPos = (last) ? ((char*)last + last->size + sizeof(memStackMarker_t)) : base;
    if (size <= (size_t)(stack->memLast - markerPos))
    {
        memStackMarker_t* marker = (memStackMarker_t*)markerPos;
        unsigned int markerBit = (((char*)marker - base) & UINT32_MAX) / ((unsigned int)MEM_ALIGN_DEFAULT);
        assert(!BIT_TEST(stack->memLast, markerBit));
        result = ((char*)marker) + sizeof(memStackMarker_t);
        BIT_SET(stack->memLast, markerBit);
        marker->prev = stack->lastMarker;
        marker->size = size - sizeof(memStackMarker_t);
        stack->lastMarker = marker;
    }
    return result;
}

void memStackFree(memStackAlloc_t* stack, void* mem)
{
    char* base = ((char*)stack) + sizeof(memStackAlloc_t);
    char* addr = (mem) ? ((char*)mem) - sizeof(memStackMarker_t) : base;
    assert(IS_ALIGNED(addr) && (addr > base) && (addr < stack->memLast));
    unsigned int markerBit = ((addr - base) & UINT32_MAX) / ((unsigned int)MEM_ALIGN_DEFAULT);
    assert(BIT_TEST(stack->memLast, markerBit));
    memStackMarker_t* marker = (memStackMarker_t*)addr;
    stack->lastMarker = marker->prev;
    BIT_UNSET(stack->memLast, markerBit);
}

void memStackDestroy(memStackAlloc_t** outStack)
{
    assert(outStack);
    memStackAlloc_t* stack = *outStack;
    if (stack)
    {
        free(stack);
        *outStack = NULL;
    }
}
