#pragma once

#include "vulkan10.h"

#include <stdbool.h>
#include <stdint.h>

#include <core/math.h>

typedef enum
{
    GFX_FORMAT_BRGA8 = VK_FORMAT_B8G8R8A8_UNORM,
    GFX_FORMAT_D24S8 = VK_FORMAT_D24_UNORM_S8_UINT,
    GFX_DEFAULT_COLOR_FORMAT = GFX_FORMAT_BRGA8,
    GFX_DEFAULT_DEPTH_STENCIL_FORMAT = GFX_FORMAT_D24S8
} gfxDataFormat_t;

typedef struct
{
    VkDevice device;
    VkSurfaceKHR surface;
    VkPhysicalDeviceMemoryProperties memProps;
    VkQueue drawQueue, transferQueue;
    VkCommandBuffer cmdBuffer;
} gfxContext_t;

typedef struct
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

struct GLFWwindow;

VkResult gfxCreateDevice(gfxContext_t* gfx, struct GLFWwindow* window);

void gfxDestroyDevice(gfxContext_t* gfx);

VkResult gfxCreateTexture(gfxContext_t* gfx, gfxTexture_t* texture);

void gfxDestroyTexture(gfxContext_t* gfx, gfxTexture_t* texture);
