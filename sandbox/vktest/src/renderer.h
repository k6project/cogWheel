#pragma once

#include "vulkan10.h"

#include <stdbool.h>
#include <stdint.h>

#include <core/math.h>

typedef enum gfxDataFormat_t
{
    GFX_FORMAT_BRGA8 = VK_FORMAT_B8G8R8A8_UNORM,
    GFX_FORMAT_D24S8 = VK_FORMAT_D24_UNORM_S8_UINT,
	GFX_FORMAT_GRAYSCALE  = VK_FORMAT_R8_UNORM,
    GFX_DEFAULT_COLOR_FORMAT = GFX_FORMAT_BRGA8,
    GFX_DEFAULT_DEPTH_STENCIL_FORMAT = GFX_FORMAT_D24S8
} gfxDataFormat_t;

typedef struct gfxBuffer_t
{
	VkBuffer handle;
	VkDeviceMemory memory;
	void* hostPtr;
	uint32_t size;
	uint32_t flags :27;
	bool upload    : 1;
	bool uniform   : 1;
	bool vertex    : 1;
	bool index     : 1;
	bool ownGpuMem : 1;
} gfxBuffer_t;

typedef struct gfxTexture_t
{
    VkImage image;
    VkImageView handle;
    VkDeviceMemory memory;
    uint32_t width        :16;
    uint32_t height       :16;
    gfxDataFormat_t format:24;
    uint32_t numMips      : 4;
    bool renderTarget     : 1;
    bool sampledTexture   : 1;
    bool ownGpuMem        : 1;
    bool hasPendingData   : 1;
    void* imageData;
    size_t imageDataSize;
} gfxTexture_t;

typedef struct gfxContext_t
{
	VkDevice device;
	VkSurfaceKHR surface;
	VkPhysicalDeviceMemoryProperties memProps;
	VkQueue drawQueue, transferQueue;
	VkCommandBuffer cmdBuffer;
	gfxBuffer_t stagingBuffer;
} gfxContext_t;

struct GLFWwindow;

VkResult gfxCreateBuffer(gfxContext_t* gfx, gfxBuffer_t* buffer);

void gfxDestroyBuffer(gfxContext_t* gfx, gfxBuffer_t* buffer);

VkResult gfxCreateTexture(gfxContext_t* gfx, gfxTexture_t* texture);

void gfxDestroyTexture(gfxContext_t* gfx, gfxTexture_t* texture);

VkResult gfxCreateDevice(gfxContext_t* gfx, struct GLFWwindow* window);

void gfxDestroyDevice(gfxContext_t* gfx);
