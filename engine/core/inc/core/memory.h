#pragma once

#include <stddef.h>

#define MEM_ALIGN_DEFAULT 16

#ifdef WIN32
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

