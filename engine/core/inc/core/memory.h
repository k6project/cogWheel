#pragma once

#include <stddef.h>

typedef struct memStackMarker_t
{
    struct memStackMarker_t* prev;
    size_t size;
} memStackMarker_t

typedef struct memStackAlloc_t
{
    char* memBase;
    char* memDynamic;
    memStackMarker_t* lastMarker;
} memStackAlloc_t;

void* memStackAllocStatic(memStackAlloc_t* stack, size_t size);

void* memStackAlloc(memStackAlloc_t* stack, size_t size, memStackMarker_t* marker);

void memStackFree(memStackMarker_t* marker);

/*
 maker: pointer to prev marker + allocation size (16 bytes)
 if no dynamic allocs made:
     create first marker
 else:
     take last marker, add allocation size of last marker -> place new marker, set size
 */

