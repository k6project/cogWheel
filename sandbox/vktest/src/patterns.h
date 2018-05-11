#pragma once

#include <stddef.h>
#include <stdint.h>

struct gfxTexture_t;

void checkerboard(struct gfxTexture_t*);

void voronoiNoise(struct gfxTexture_t* texture, uint32_t gridW, uint32_t gridH);
