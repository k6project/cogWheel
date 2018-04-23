#include "patterns.h"

#include <stdlib.h>
#include <assert.h>

#include "renderer.h"

static int lcd(uint32_t a, uint32_t b)
{
    uint32_t max = (a > b) ? a : b;
    uint32_t min = (a > b) ? b : a;
    while(max != min)
    {
        uint32_t tmp = max - min;
        max = (tmp < min) ? min : tmp;
        min = (tmp < min) ? tmp : min;
    }
    return max;
}

void checkerboard(struct gfxTexture_t* texture)
{
    size_t step = lcd(texture->width, texture->height);
    size_t rows = texture->height / step;
    while (rows < 8)
    {
        step >>= 1;
        rows <<= 1;
    }
    char* mem = (char*)texture->imageData;
    size_t cols = texture->width / step, ppr = step * cols;
    size_t dataSize = ppr * rows * step;
    if (!texture->imageData)
    {
        mem = (char*)malloc(dataSize);
        texture->imageDataSize = dataSize;
    }
    else
    {
        assert(texture->imageDataSize >= dataSize);
        texture->imageDataSize = dataSize;
    }
    for (size_t i = 0; i < dataSize; i++)
    {
        size_t tmp = i % ppr;
        size_t row = i / ppr;
        size_t col = tmp / step;
        size_t token = (col & 1) + (row & 1);
        mem[i] = (token & 1) * 0xff;
    }
    texture->format = GFX_FORMAT_GRAYSCALE;
    texture->hasPendingData = true;
    texture->sampledTexture = true;
	texture->imageData = mem;
}
