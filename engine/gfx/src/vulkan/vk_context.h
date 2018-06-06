#pragma once

#include <gfx/coredefs.h>

#include <vulkan/vulkan.h>

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

extern const struct gfxApi_t_ gApi;
