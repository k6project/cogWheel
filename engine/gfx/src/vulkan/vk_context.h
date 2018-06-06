#pragma once

#ifdef _cplusplus
extern "C"
{
#endif

#include <core/memory.h>
#include <gfx/coredefs.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VKCHECK(call) do { VkResult VKRESULT = call ; ENSURE(VKRESULT==VK_SUCCESS);} while (0)
#define VKINIT(obj,tname) do { memset(&obj, 0, sizeof(obj));(obj).sType = tname; } while (0)

typedef struct
{
    VkPhysicalDevice handle;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory;
    uint32_t numFamilies;
    VkQueueFamilyProperties* families;
    uint64_t memTotalSize;
} vklDeviceInfo_t;

typedef struct
{
    uint32_t index;
    uint32_t numQueues;
    VkDeviceQueueCreateInfo* queues;
} vklDeviceSetup_t;

typedef struct
{
    void* dll;
    uint32_t numLayers;
    VkLayerProperties* layers;
    uint32_t numExtensions;
    VkExtensionProperties* extensions;
	uint32_t numRequiredLayers;
	const char** requiredLayers;
    VkInstance instance;
    uint32_t numDevices;
    vklDeviceInfo_t* deviceInfo;
} vklContext_t;

extern vklContext_t gContext;

typedef struct
{
	VkDevice id;
	memStackAlloc_t* memory;
	memObjPool_t* texturePool;
	memObjPool_t* bufferPool;
	VkSurfaceKHR surface;
	VkExtent2D surfaceSize;
	VkSurfaceTransformFlagBitsKHR transform;
	VkSurfaceFormatKHR surfFormat;
	VkPresentModeKHR presentMode;
	VkPhysicalDeviceMemoryProperties memProps;
	uint32_t queueFamily;
	uint32_t numBuffers;
	uint32_t bufferIdx;
	gfxTexture_t imgBuffers[GFX_MAX_FRAMEBUFFERS];
	gfxTexture_t backBuffer;
	VkQueue cmdQueue;
	VkCommandPool cmdPool;
	gfxBuffer_t stagingBuffer;
	VkSwapchainKHR swapChain;
	VkSemaphore canDraw;
	VkSemaphore canSwap;
	VkCommandBuffer cbPerFrame[GFX_MAX_FRAMEBUFFERS];
	VkCommandBuffer cbDraw;
	VkCommandBuffer cbTransfer;
} vklDevice_t;

extern vklDevice_t gDevice;

struct gfxTextureImpl_t
{
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;
};

struct gfxBufferImpl_t
{
	VkBuffer handle;
	VkDeviceMemory memory;
};

struct gfxShaderImpl_t
{
	VkShaderModule vertex;
	VkShaderModule hull;
	VkShaderModule domain;
	VkShaderModule geometry;
	VkShaderModule pixel;
	VkShaderModule compute;
};

struct gfxPipelineImpl_t
{
	VkPipeline handle;
};

gfxResult_t vklInitContext();
void vklDestroyContext();
VkSurfaceKHR vklCreateSurface(void* nativePtr);
gfxResult_t vklInitDevice(void* nativePtr);
void vklDestroyDevice();
gfxBuffer_t vklNewBuffer();
gfxResult_t vklInitBuffer(gfxBuffer_t buffer);
void vklDestroyBuffer(gfxBuffer_t buffer);
gfxTexture_t vklNewTexture();
gfxResult_t vklInitTexture(gfxTexture_t texture);
void vklDestroyTexture(gfxTexture_t texture);
void vklBeginFrame();
void vklUpdate(gfxTexture_t* textures, size_t numTextures, gfxBuffer_t* buffers, size_t numBuffers);
void vklClear(gfxTexture_t texture, vec4f_t color);
void vklBlit(gfxTexture_t dest, gfxTexture_t src);
void vklEndFrame();
    
#ifdef _cplusplus
}
#endif
