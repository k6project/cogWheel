#pragma once

#include <stddef.h>

struct gfxTexture_t;

void checkerboard(struct gfxTexture_t*);

void voronoiNoise(struct gfxTexture_t* texture, int gridW, int gridH);
