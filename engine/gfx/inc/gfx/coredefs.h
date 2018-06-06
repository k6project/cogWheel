#pragma once

#include <stddef.h>
#include <stdbool.h>

#include <core/math.h>
#include <core/coredefs.h>

struct gfxTextureImpl_t;
struct gfxBufferImpl_t;
struct gfxPipelineImpl_t;

enum
{
	GFX_MAX_FRAMEBUFFERS = 8,
    GFX_NUM_FRAMEBUFFERS = 3,
	GFX_SIZE_STAGING_BUFFER = (16u << 20),
	GFX_SIZE_LINEAR_ALLOC = (4u << 20)
};

typedef union
{
    struct gfxTextureImpl_t* texture;
    struct gfxBufferImpl_t* buffer;
    struct gfxPipelineImpl_t* pipeline;
} gfxResource_t;

typedef enum gfxResult_t_
{
    GFX_SUCCESS,
    GFX_ERR_UNKNOWN
} gfxResult_t;

typedef enum gfxDataFormat_t_
{
    GFX_FORMAT_BRGA8, // = VK_FORMAT_B8G8R8A8_UNORM,
    GFX_FORMAT_BGRA8_SRGB, // = VK_FORMAT_B8G8R8A8_SRGB,
    GFX_FORMAT_RGBA8, // = VK_FORMAT_R8G8B8A8_UNORM,
    GFX_FORMAT_RGBA8_SRGB, // = VK_FORMAT_R8G8B8A8_SRGB,
    GFX_FORMAT_D24S8, // = VK_FORMAT_D24_UNORM_S8_UINT,
    GFX_FORMAT_GRAYSCALE, //  = VK_FORMAT_R8_UNORM,
    GFX_DEFAULT_COLOR_FORMAT = GFX_FORMAT_BRGA8,
    GFX_DEFAULT_DEPTH_STENCIL_FORMAT = GFX_FORMAT_D24S8
} gfxDataFormat_t;

struct gfxBuffer_t_
{
    void* hostPtr;
    uint32_t size;
    uint32_t flags :27;
    bool upload    : 1;
    bool uniform   : 1;
    bool vertex    : 1;
    bool index     : 1;
    bool ownGpuMem : 1;
    gfxResource_t impl;
};

struct gfxTexture_t_
{
    uint32_t width : 16;
    uint32_t height : 16;
    gfxDataFormat_t format : 24;
    uint32_t numMips : 4;
    bool renderTarget : 1;
    bool sampledTexture : 1;
    bool ownGpuMem : 1;
    bool hasPendingData : 1;
    size_t imageDataSize;
    void* imageData;
    gfxResource_t impl;
};

typedef struct gfxBuffer_t_* gfxBuffer_t;
typedef struct gfxTexture_t_* gfxTexture_t;

typedef struct gfxApi_t_
{
	gfxResult_t(*const initDevice)(void* nativePtr);
    void (*const destroyDevice)();
    gfxBuffer_t (*const newBuffer)();
    gfxResult_t (*const initBuffer)(gfxBuffer_t buffer);
    void (*const destroyBuffer)(gfxBuffer_t buffer);
	gfxTexture_t (*const newTexture)();
	gfxResult_t (*const initTexture)(gfxTexture_t texture);
	void (*const destroyTexture)(gfxTexture_t texture);
	void (*const beginFrame)();
	void (*const update)(gfxTexture_t* textures, size_t numTextures, gfxBuffer_t* buffers, size_t numBuffers);
	void (*const clear)(gfxTexture_t texture, vec4f_t color);
	void (*const blit)(gfxTexture_t dest, gfxTexture_t src);
	void (*const endFrame)();
}* const gfxApi_t;

gfxApi_t gfxApi();
