#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
#define VULKAN_API_GOBAL(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_INSTANCE(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_DEVICE(proc) extern PFN_vk ## proc vk ## proc;
#include "vkproc.inl.h"

typedef struct
{
    char name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    VkPhysicalDeviceType type;
} vkDeviceInfo_t;

typedef struct
{
    void* dll;
    uint32_t numLayers;
    VkLayerProperties* layers;
    uint32_t numExtensions;
    VkExtensionProperties* extensions;
    uint32_t numDevices;
    VkPhysicalDevice* devices;
    vkDeviceInfo_t* deviceInfo;
} vkContext_t;
