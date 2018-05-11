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

/*

ORIGINAL GLSL:

const int Seed = 0;
const int GridSize = 5;
const int MaxIndex = GridSize - 1;
const float NormUInt16 = 0.00001526;

int coordHash(ivec2 block)
{
	int result = Seed;
	result += block.x;
	result += (result << 10);
	result ^= (result >> 6 );
	result += block.y;
	result += (result << 10);
	result ^= (result >> 6 );
	return result;
}

int nextRandom(int prev)
{
	return (((prev * 1103515245 + 12345) % 2147483647) & 0x7fffffff) % 65536;
}

float fpNearest(ivec2 block, vec2 offset)
{
	int seed = coordHash(block) ^ Seed;

	int x_int = nextRandom(seed);
	int y_int = nextRandom(x_int);
	vec2 featurePoint = vec2(float(x_int), float(y_int)) * NormUInt16;
	float result = length(featurePoint - offset);

	x_int = nextRandom(y_int);
	y_int = nextRandom(x_int);
	featurePoint = vec2(float(x_int), float(y_int)) * NormUInt16;
	result = min(result, length(featurePoint - offset));

	return clamp(result, 0.0, 1.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;

	vec2 gridCoord = uv * float(GridSize);
	ivec2 block = ivec2(floor(gridCoord));
	vec2 blockOffset = fract(gridCoord);
	float noiseValue = fpNearest(block, blockOffset);

	ivec4 nblock = ivec4(
		(block.x == 0) ? MaxIndex : block.x - 1,
		(block.x == MaxIndex) ? 0 : block.x + 1,
		(block.y == 0) ? MaxIndex : block.y - 1,
		(block.y == MaxIndex) ? 0 : block.y + 1
	);
	vec4 offs = blockOffset.xxyy + vec4(1.0, -1.0, 1.0, -1.0);


	noiseValue = min(noiseValue, fpNearest(ivec2(block.x, nblock.w), vec2(blockOffset.x, offs.w)));
	noiseValue = min(noiseValue, fpNearest(ivec2(block.x, nblock.z), vec2(blockOffset.x, offs.z)));
	noiseValue = min(noiseValue, fpNearest(ivec2(nblock.y, block.y), vec2(offs.y, blockOffset.y)));
	noiseValue = min(noiseValue, fpNearest(ivec2(nblock.x, block.y), vec2(offs.x, blockOffset.y)));
	noiseValue = min(noiseValue, fpNearest(nblock.yw, vec2(offs.y, offs.w)));
	noiseValue = min(noiseValue, fpNearest(nblock.yz, vec2(offs.y, offs.z)));
	noiseValue = min(noiseValue, fpNearest(nblock.xw, vec2(offs.x, offs.w)));
	noiseValue = min(noiseValue, fpNearest(nblock.xz, vec2(offs.x, offs.z)));

	noiseValue = clamp(noiseValue, 0.0, 1.0);
	vec3 c1 = (1.0 - noiseValue) * vec3(1.0, 0.0, 0.0);
	vec3 c2 = noiseValue * vec3(1.0, 1.0, 0.0);
	fragColor = vec4(c1 + c2, 1.0);
}

*/

typedef struct ngVoronoi_t
{
    vec2i_t gridSize;
    uint32_t numFeaturePoints;
    vec2f_t* featurePoints;
} ngVoronoi_t;

void ngInitVoronoi(ngVoronoi_t** gen, vec2i_t gridSize, uint32_t fpPerTile)
{
    assert(*gen == NULL);
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
}

static float ngVoronoiTileProc(ngVoronoi_t* gen, vec4i_t point)
{
    float u = point[0] / ((float)gen->gridSize[0]);
    float v = point[1] / ((float)gen->gridSize[1]);
    //int32_t (point[0] + point[1] * gen->gridSize[0]) * ;
    return 0.f;
}

void ngMakeVoronoi(ngVoronoi_t* gen, gfxTexture_t* texture)
{
    
}

static float voronoiTile(int32_t data[4], int32_t gridW, int32_t gridH, prng_t prng)
{
    float pY = data[2] / ((float)gridH);
    float pX = data[3] / ((float)gridW);
    float fpX = mathRandomf(prng);
    float fpY = mathRandomf(prng);
    float distance = (fpX - pX) * (fpX - pX) + (fpY - pY) * (fpY - pY);
    return (distance > 1.f) ? 1.f : sqrtf(distance);
}

void voronoiNoise(struct gfxTexture_t* texture, int gridW, int gridH)
{
    const uint32_t fpPerTile = 2;
    assert(gridW > 0 && gridH > 0);
    ngVoronoi_t* gen = (ngVoronoi_t*)malloc(sizeof(ngVoronoi_t) + gridW * gridH * sizeof(vec2f_t) * fpPerTile);
    
    prng_t prng = MATH_PRNG(0);
    assert(gridW > 0 && gridH > 0);
    char* mem = (char*)texture->imageData;
    size_t dataSize = texture->width * texture->height;
    if (!texture->imageData)
    {
        mem = (char*)malloc(dataSize);
        texture->imageData = mem;
        texture->imageDataSize = dataSize;
    }
    int blockW = texture->width / gridW;
    int blockH = texture->height / gridH;
    for (size_t i = 0; i < dataSize; i++)
    {
        int row = (i / texture->width) & ((int)-1);
        int col = i % texture->width;
        int blockR = row / blockH, blockY = row % blockH;
        int blockC = col / blockW, blockX = col % blockW;
        struct { int center[4], top[4], bottom[4], right[4], left[4]; } blocks =
        {
            { blockR, blockC, blockY, blockX },
            { (blockR) ? blockR - 1 : gridH - 1, blockC, blockY + blockH, blockX },
            { (blockR + 1 < gridH) ? blockR + 1 : 0, blockC, blockY - blockH, blockX },
            { blockR, (blockC + 1 < gridW) ? blockC + 1 : 0, blockY, blockX - blockW },
            { blockR, (blockC) ? blockC - 1 : gridW - 1, blockY, blockX + blockW }
        };
        float noise = 1.f;
        noise = fminf(noise, voronoiTile(blocks.center, blockW, blockH, prng));
        /*noise = fminf(noise, voronoiTile(blocks.top, blockW, blockH));
        noise = fminf(noise, voronoiTile(blocks.bottom, blockW, blockH));
        noise = fminf(noise, voronoiTile(blocks.right, blockW, blockH));
        noise = fminf(noise, voronoiTile(blocks.left, blockW, blockH));*/
        char value = (char)roundf(255.f * noise);
        mem[i] = value;
    }
    texture->format = GFX_FORMAT_GRAYSCALE;
    texture->hasPendingData = true;
    texture->sampledTexture = true;
}
