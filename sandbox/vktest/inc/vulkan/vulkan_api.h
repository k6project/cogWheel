#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VULKAN_API_GOBAL(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_INSTANCE(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_DEVICE(proc) extern PFN_vk ## proc vk ## proc;
#include "vkproc.inl.h"

typedef struct
{
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
    VkPhysicalDevice* devices;
    vklDeviceInfo_t* deviceInfo;
} vklContext_t;

typedef VkResult (*vklDeviceSetupProc_t)(void*, vklDeviceSetup_t*, const vklDeviceInfo_t*, uint32_t);

VkDevice vklCreateLogicalDevice(vklDeviceSetupProc_t, void*);