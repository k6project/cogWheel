#include "patterns.h"

#include <stdlib.h>

static int lcd(int a, int b)
{
    int max = (a > b) ? a : b;
    int min = (a > b) ? b : a;
    while(max != min)
    {
        int tmp = max - min;
        max = (tmp < min) ? min : tmp;
        min = (tmp < min) ? tmp : min;
    }
    return max;
}

void checkerboard(int w, int h, void** outMem, size_t* outSize)
{
    size_t step = lcd(w, h);
    size_t rows = h / step;
    while (rows < 8)
    {
        step >>= 1;
        rows <<= 1;
    }
    size_t cols = w / step, ppr = step * cols;
    *outSize = ppr * rows * step;
    char* mem = (!*outMem) ? (char*)malloc(*outSize) : (char*)(*outMem);
    for (size_t i = 0; i < *outSize; i++)
    {
        size_t tmp = i % ppr;
        size_t row = i / ppr;
        size_t col = tmp / step;
        size_t token = (col & 1) + (row & 1);
        mem[i] = (token & 1) * 0xff;
    }
	*outMem = mem;
}
