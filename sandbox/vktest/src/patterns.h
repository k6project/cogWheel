#pragma once

#include <stddef.h>
#include <stdint.h>

struct gfxTexture_t_;

void checkerboard(struct gfxTexture_t_*);

void voronoiNoise(struct gfxTexture_t_* texture, uint32_t gridW, uint32_t gridH);
