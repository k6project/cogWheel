#pragma once

#include <stddef.h>

#define MEM_ALIGN_DEFAULT 16

typedef struct memStackMarker_t
{
    struct memStackMarker_t* prev;
    size_t size;
} memStackMarker_t;

typedef struct memStackAlloc_t
{
    char* memBase;
    char* memLast;
    memStackMarker_t* lastMarker;
    size_t freeBytes;
} memStackAlloc_t;

void* memStackAlloc(memStackAlloc_t* stack, size_t size);

void memStackFree(memStackAlloc_t* stack, void* mem);

/*
 maker: pointer to prev marker + allocation size (16 bytes)
 if no dynamic allocs made:
     create first marker
 else:
     take last marker, add allocation size of last marker -> place new marker, set size
 
 
 
 */

