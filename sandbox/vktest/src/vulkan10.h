#pragma once

#ifdef _cplusplus
extern "C"
{
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VULKAN_API_GOBAL(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_INSTANCE(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_DEVICE(proc) extern PFN_vk ## proc vk ## proc;
#include "vkproc.inl.h"

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
    VkInstance instance;
    uint32_t numDevices;
    vklDeviceInfo_t* deviceInfo;
} vklContext_t;

void vklInitialize(const char*);

VkSurfaceKHR vklCreateSurface(void*);

void vklDestroySurface(VkSurfaceKHR);

typedef VkResult (*vklDeviceSetupProc_t)(void*, vklDeviceSetup_t*, const vklDeviceInfo_t*, uint32_t);

VkDevice vklCreateDevice(vklDeviceSetupProc_t, void*);

VkResult vklMemAlloc(VkDevice device,
    const VkPhysicalDeviceMemoryProperties* props,
    const VkMemoryRequirements* reqs,
    const VkMemoryPropertyFlags flags,
    VkDeviceMemory* memory);

void vklShutdown();

#define VKCHECK(call) do { VkResult VKRESULT = call ; assert(VKRESULT==VK_SUCCESS);} while (0)
#define VKINIT(obj,tname) do { memset(&obj, 0, sizeof(obj));(obj).sType = tname; } while (0)

#ifdef _cplusplus
}
#endif
