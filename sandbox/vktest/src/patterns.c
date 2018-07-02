#include "patterns.h"

#include <stdlib.h>

#include <gfx/coredefs.h>

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

void checkerboard(struct gfxTexture_t_* texture)
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
        CHECK(texture->imageDataSize >= dataSize);
        texture->imageDataSize = dataSize;
    }
    for (size_t i = 0; i < dataSize; i++)
    {
        size_t tmp = i % ppr;
        size_t col = tmp / step;
		size_t row = (i / ppr) / step;
        size_t token = (col & 1) + (row & 1);
        mem[i] = (token & 1) * 0xff;
    }
    texture->format = GFX_FORMAT_GRAYSCALE;
    texture->hasPendingData = true;
    texture->sampledTexture = true;
	texture->imageData = mem;
}

/**************************************************/

struct ngPerlin_t_
{
    vec2u_t gridSize;
    vec2f_t* gradients;
};

typedef struct ngPerlin_t_ ngPerlin_t;

static float ngPerlinEval(ngPerlin_t* gen, vec2u_t pos)
{
    return 0.f;
}

/**************************************************/

typedef struct ngVoronoi_t
{
    vec2u_t gridSize;
	vec2i_t blockSize;
    uint32_t numFeaturePoints;
	uint32_t fpPerTile;
    vec2f_t* featurePoints;
} ngVoronoi_t;

void ngInitVoronoi(ngVoronoi_t** gen, vec2u_t gridSize, uint32_t fpPerTile)
{
    CHECK(*gen == NULL);
    prng_t prng = MATH_PRNG(0);
    uint32_t numFeaturePoints = gridSize[0] * gridSize[1] * fpPerTile;
    ngVoronoi_t* tmp = (ngVoronoi_t*)malloc(sizeof(ngVoronoi_t) + numFeaturePoints * sizeof(vec2f_t));
    tmp->featurePoints = (vec2f_t*)((char*)tmp + sizeof(ngVoronoi_t));
    tmp->numFeaturePoints = numFeaturePoints;
    for (uint32_t i = 0; i < numFeaturePoints; i++)
    {
        tmp->featurePoints[i][0] = mathRandomf(prng);
        tmp->featurePoints[i][1] = mathRandomf(prng);
    }
	MATH_VEC2_MOV(tmp->gridSize, gridSize);
	tmp->fpPerTile = fpPerTile;
	*gen = tmp;
}

static float ngVoronoiTileProc(ngVoronoi_t* gen, vec2i_t point, vec2u_t block)
{
	float result = 1.f;
    float u = point[0] / ((float)gen->blockSize[0]);
    float v = point[1] / ((float)gen->blockSize[1]);
    uint32_t fpBase = (block[0] + block[1] * gen->gridSize[0]) * gen->fpPerTile;
	for (uint32_t i = 0; i < gen->fpPerTile; i++)
	{
		vec2f_t distVec;
		mathVec2fSub(distVec, gen->featurePoints[fpBase + i], (vec2f_t){ u, v });
		float dist = mathVec2fLength(distVec);
		if (result > dist)
		{
			result = dist;
		}
	}
    return result;
}

void ngMakeVoronoi(ngVoronoi_t* gen, gfxTexture_t texture)
{
	char* mem = (char*)texture->imageData;
	size_t dataSize = texture->width * texture->height;
	if (!texture->imageData)
	{
		mem = (char*)malloc(dataSize);
		texture->imageData = mem;
		texture->imageDataSize = dataSize;
	}
	gen->blockSize[0] = texture->width / (int32_t)gen->gridSize[0];
	gen->blockSize[1] = texture->height / (int32_t)gen->gridSize[1];
	for (size_t i = 0; i < dataSize; i++)
	{
		float result = 1.f;
		uint32_t x = i % texture->width;
		uint32_t y = (i / texture->width) & UINT32_MAX;
        /*x = (x + 100) % texture->width;
        y = (y + 100) % texture->height;*/
		uint32_t blockR = y / gen->blockSize[1];
		uint32_t blockC = x / gen->blockSize[0];
		int32_t blockX = (int32_t)(x % gen->blockSize[0]);
		int32_t blockY = (int32_t)(y % gen->blockSize[1]);
		int32_t topY = blockY + gen->blockSize[1];
		uint32_t topR = (blockR) ? blockR - 1 : gen->gridSize[1] - 1;
		int32_t bottomY = blockY - gen->blockSize[1];
		uint32_t bottomR = (blockR + 1 < gen->gridSize[1]) ? blockR + 1 : 0;
		int32_t rightX = blockX - gen->blockSize[0];
		uint32_t rightC = (blockC + 1 < gen->gridSize[0]) ? blockC + 1 : 0;
		int32_t leftX = blockX + gen->blockSize[0];
		uint32_t leftC =  (blockC) ? blockC - 1 : gen->gridSize[0] - 1;
		struct { vec2i_t point; vec2u_t block; } blocks[] = 
		{
			/* center       */ {{blockX, blockY}, {blockC, blockR}},
			/* top          */ {{blockX, topY}, {blockC, topR}},
			/* bottom       */ {{blockX, bottomY}, {blockC, bottomR}},
			/* right        */ {{rightX, blockY}, {rightC, blockR}},
			/* left         */ {{leftX, blockY}, {leftC, blockR}},
			/* top-left     */ {{leftX, topY}, {leftC, topR}},
			/* bottom-left  */ {{leftX, bottomY}, {leftC, bottomR}},
			/* top-right    */ {{rightX, topY}, {rightC, topR}},
			/* bottom-right */ {{rightX, bottomY}, {rightC, bottomR}}
		};
		for (int j = 0; j < 9; j++)
		{
			float noise = ngVoronoiTileProc(gen, blocks[j].point, blocks[j].block);
			if (noise < result)
			{
				result = noise;
			}
		}
		mem[i] = (char)(roundf(255.f * result));
	}
	texture->format = GFX_FORMAT_GRAYSCALE;
	texture->hasPendingData = true;
	texture->sampledTexture = true;
}

void voronoiNoise(gfxTexture_t texture, uint32_t gridW, uint32_t gridH)
{
    CHECK(gridW > 0 && gridH > 0);
	ngVoronoi_t* gen = NULL;
	ngInitVoronoi(&gen, (vec2u_t){gridW, gridH}, 1);
	ngMakeVoronoi(gen, texture);
	free(gen);
}
