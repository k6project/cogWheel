#pragma once

#ifdef _cplusplus
extern "C"
{
#endif

#include <core/coredefs.h>

#define MEM_ALIGN_DEFAULT 16u
#define MEM_ALIGN_MASK (MEM_ALIGN_DEFAULT-1)
#define MEM_ALIGNED(n) ((n+MEM_ALIGN_MASK)&(~MEM_ALIGN_MASK))

#ifdef _MSC_VER
#define ALIGNED(n) __declspec( align( n ) )
#else
#define ALIGNED(n) __attribute__((aligned( n )))
#endif

struct memStackMarker_t;

struct memStackAlloc_t;
typedef struct memStackAlloc_t memStackAlloc_t;

void memStackInit(memStackAlloc_t** outStack, size_t size);

void* memStackAlloc(memStackAlloc_t* stack, size_t size);

void memStackFree(memStackAlloc_t* stack, void* mem);

void memStackDestroy(memStackAlloc_t** outStack);

struct memObjPool_t;
typedef struct memObjPool_t memObjPool_t;

void memObjPoolInit(memObjPool_t** outPool, size_t objSize, size_t count);

void* memObjPoolGet(memObjPool_t* pool);

void memObjPoolPut(memObjPool_t* pool, void* obj);

void memObjPoolDestroy(memObjPool_t** outPool);
    
size_t memObjPoolGetStride(memObjPool_t* pool);

#ifdef _cplusplus
}
#endif
