#include "vulkan10.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
#define VULKAN_API_GOBAL(proc) PFN_vk ## proc vk ## proc = NULL;
#define VULKAN_API_INSTANCE(proc) PFN_vk ## proc vk ## proc = NULL;
#define VULKAN_API_DEVICE(proc) PFN_vk ## proc vk ## proc = NULL;
#include "vkproc.inl.h"

static const char* VK_REQUIRED_LAYERS[] =
{
    "VK_LAYER_LUNARG_standard_validation"
};

static const char* VK_REQUIRED_EXTENSIONS[] =
{
    VK_KHR_SURFACE_EXTENSION_NAME
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    , VK_MVK_MACOS_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    , VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#ifdef VK_DEBUG
    , VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
};

static const char* VK_REQUIRED_DEVICE_EXTENSIONS[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const uint32_t VK_NUM_REQUIRED_LAYERS = sizeof(VK_REQUIRED_LAYERS) / sizeof(const char*);
static const uint32_t VK_NUM_REQUIRED_EXTENSIONS = sizeof(VK_REQUIRED_EXTENSIONS) / sizeof(const char*);
static const uint32_t VK_NUM_REQUIRED_DEVICE_EXTENSIONS = sizeof(VK_REQUIRED_DEVICE_EXTENSIONS) / sizeof(const char*);

static vklContext_t vkCtx_;

void vklInitialize(const char* appArg)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    vkCtx_.dll = LoadLibrary("vulkan-1.dll");
#else
    const char* term = strrchr(appArg, '/');
    static const char libName[] = "libvulkan.dylib";
    if (term)
    {
        int length = term - appArg + 1;
        char* cwd = (char*)malloc(length + sizeof(libName));
        memcpy(cwd, appArg, length);
        memcpy(cwd + length, libName, sizeof(libName));
        vkCtx_.dll = dlopen(cwd, RTLD_LOCAL | RTLD_NOW);
    }
    else
    {
        vkCtx_.dll = dlopen(libName, RTLD_LOCAL | RTLD_NOW);
    }
    if (!vkCtx_.dll)
    {
        printf("%s\n", dlerror());
        assert(0);
    }
#endif
    VkResult result = VK_SUCCESS;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vkCtx_.dll, "vkGetInstanceProcAddr");
#else
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vkCtx_.dll, "vkGetInstanceProcAddr");
#endif
    assert(vkGetInstanceProcAddr);
#define VULKAN_API_GOBAL(proc) \
    assert(vk ## proc = ( PFN_vk ## proc )vkGetInstanceProcAddr( NULL, "vk" #proc ));
#include "vkproc.inl.h"
    result = vkEnumerateInstanceLayerProperties(&vkCtx_.numLayers, NULL);
    assert(result == VK_SUCCESS);
#ifdef VK_DEBUG
    if (vkCtx_.numLayers > 0)
    {
        vkCtx_.layers = (VkLayerProperties*)malloc(vkCtx_.numLayers * sizeof(VkLayerProperties));
        result = vkEnumerateInstanceLayerProperties(&vkCtx_.numLayers, vkCtx_.layers);
        assert(result == VK_SUCCESS);
        for (uint32_t i = 0; i < vkCtx_.numLayers; i++)
        {
            printf("%s\n", vkCtx_.layers[i].layerName);
        }
    }
#endif
    result = vkEnumerateInstanceExtensionProperties(NULL, &vkCtx_.numExtensions, NULL);
    assert(result == VK_SUCCESS);
#ifdef VK_DEBUG
    if (vkCtx_.numExtensions > 0)
    {
        vkCtx_.extensions = (VkExtensionProperties*)malloc(vkCtx_.numExtensions * sizeof(VkExtensionProperties));
        result = vkEnumerateInstanceExtensionProperties(NULL, &vkCtx_.numExtensions, vkCtx_.extensions);
        assert(result == VK_SUCCESS);
        for (uint32_t i = 0; i < vkCtx_.numExtensions; i++)
        {
            printf("%s\n", vkCtx_.extensions[i].extensionName);
        }
    }
#endif
    VkApplicationInfo appInfo;
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType =VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = PROGRAM_NAME;
    appInfo.applicationVersion = PROGRAM_VERSION;
    appInfo.pEngineName = ENGINE_NAME;
    appInfo.engineVersion = ENGINE_VERSION;
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 69);
    VkInstanceCreateInfo info;
    memset(&info, 0, sizeof(VkInstanceCreateInfo));
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &appInfo;
    info.enabledLayerCount = VK_NUM_REQUIRED_LAYERS;
    info.ppEnabledLayerNames = VK_REQUIRED_LAYERS;
    info.enabledExtensionCount = VK_NUM_REQUIRED_EXTENSIONS;
    info.ppEnabledExtensionNames = VK_REQUIRED_EXTENSIONS;
    result = vkCreateInstance(&info, NULL, &vkCtx_.instance);
    assert(result == VK_SUCCESS);
#define VULKAN_API_INSTANCE(proc) \
    assert(vk ## proc = ( PFN_vk ## proc )vkGetInstanceProcAddr( vkCtx_.instance, "vk" #proc ));
#include "vkproc.inl.h"
    result = vkEnumeratePhysicalDevices(vkCtx_.instance, &vkCtx_.numDevices, NULL);
    assert(result == VK_SUCCESS);
    if (vkCtx_.numDevices > 0)
    {
        vkCtx_.deviceInfo = (vklDeviceInfo_t*)calloc(vkCtx_.numDevices, sizeof(vklDeviceInfo_t));
        VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(vkCtx_.numDevices * sizeof(VkPhysicalDevice));
        result = vkEnumeratePhysicalDevices(vkCtx_.instance, &vkCtx_.numDevices, devices);
        assert(result == VK_SUCCESS);
        for (uint32_t i = 0; i < vkCtx_.numDevices; i++)
        {
            vkCtx_.deviceInfo[i].handle = devices[i];
            vkGetPhysicalDeviceFeatures(devices[i], &vkCtx_.deviceInfo[i].features);
            vkGetPhysicalDeviceProperties(devices[i], &vkCtx_.deviceInfo[i].properties);
            vkGetPhysicalDeviceMemoryProperties(devices[i], &vkCtx_.deviceInfo[i].memory);
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &vkCtx_.deviceInfo[i].numFamilies, NULL);
            if (vkCtx_.deviceInfo[i].numFamilies > 0)
            {
                uint32_t numFamilies = vkCtx_.deviceInfo[i].numFamilies;
                vkCtx_.deviceInfo[i].families = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * numFamilies);
                vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &vkCtx_.deviceInfo[i].numFamilies, vkCtx_.deviceInfo[i].families);
            }
            for (uint32_t j = 0; j < vkCtx_.deviceInfo[i].memory.memoryHeapCount; j++)
            {
                vkCtx_.deviceInfo[i].memTotalSize += vkCtx_.deviceInfo[i].memory.memoryHeaps[j].size;
            }
        }
        free(devices);
    }
}

VkSurfaceKHR vklCreateSurface(void* handle)
{
    VkResult result = VK_SUCCESS;
    VkSurfaceKHR surface = NULL;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.pNext = NULL;
    info.flags = 0;
    info.hinstance = GetModuleHandle(NULL);
    info.hwnd = handle;
    result = vkCreateWin32SurfaceKHR(vkCtx_.instance, &info, NULL, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    VkMacOSSurfaceCreateInfoMVK info;
    info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    info.pNext = NULL;
    info.pView = handle;
    info.flags = 0;
    result = vkCreateMacOSSurfaceMVK(vkCtx_.instance, &info, NULL, &surface);
#endif
    assert(result == VK_SUCCESS);
    return surface;
}

void vklDestroySurface(VkSurfaceKHR surface)
{
    vkDestroySurfaceKHR(vkCtx_.instance, surface, NULL);
}

VkDevice vklCreateDevice(vklDeviceSetupProc_t setupProc, void* context)
{
    vklDeviceSetup_t setup;
    if (setupProc(context, &setup, vkCtx_.deviceInfo, vkCtx_.numDevices) != VK_SUCCESS)
    {
        return NULL;
    }
    VkDevice device = NULL;
    VkDeviceCreateInfo createInfo;
    VkPhysicalDevice gpu = vkCtx_.deviceInfo[setup.index].handle;
    memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = setup.numQueues;
    createInfo.pQueueCreateInfos = setup.queues;
    createInfo.enabledExtensionCount = VK_NUM_REQUIRED_DEVICE_EXTENSIONS;
    createInfo.ppEnabledExtensionNames = VK_REQUIRED_DEVICE_EXTENSIONS;
    createInfo.ppEnabledLayerNames = VK_REQUIRED_LAYERS;
    createInfo.enabledLayerCount = VK_NUM_REQUIRED_LAYERS;
    assert(vkCreateDevice(gpu, &createInfo, NULL, &device) == VK_SUCCESS);
#define VULKAN_API_DEVICE(proc) \
    assert(vk ## proc = ( PFN_vk ## proc )vkGetDeviceProcAddr( device, "vk" #proc ));
#include "vkproc.inl.h"
    return device;
}

VkResult vklMemAlloc(VkDevice device,
                     const VkPhysicalDeviceMemoryProperties* props,
                     const VkMemoryRequirements* reqs,
                     const VkMemoryPropertyFlags flags,
                     VkDeviceMemory* memory)
{
    uint32_t type = VK_MAX_MEMORY_TYPES;
    for (uint32_t i = 0; i < props->memoryTypeCount; i++)
    {
        if ((reqs->memoryTypeBits & (1 << i)) && ((props->memoryTypes[i].propertyFlags & flags) == flags))
        {
            type = i;
            break;
        }
    }
    if (type < props->memoryTypeCount)
    {
        VkMemoryAllocateInfo allocInfo;
        memset(&allocInfo, 0, sizeof(allocInfo));
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = reqs->size;
        allocInfo.memoryTypeIndex = type;
        return vkAllocateMemory(device, &allocInfo, NULL, memory);
    }
    return VK_NOT_READY;
}

void vklShutdown()
{
    if (vkCtx_.dll)
    {
#ifdef VK_USE_PLATFORM_WIN32_KHR
        FreeLibrary(vkCtx_.dll);
#else
        dlclose(vkCtx_.dll);
#endif
    }
    free(vkCtx_.deviceInfo);
    free(vkCtx_.extensions);
    free(vkCtx_.layers);
}
