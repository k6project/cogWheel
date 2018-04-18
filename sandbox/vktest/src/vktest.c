#include <stdio.h>
#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_api.h>

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
vklContext_t* const vkCtx = &vkCtx_;

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

/* TEST APPLICATION CODE */

#define VK_MAX_SURFACE_FORMATS 8u

typedef struct
{
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCaps;
    uint32_t numPresentModes;
    uint32_t numSurfFormats;
    VkPresentModeKHR presentModes[VK_PRESENT_MODE_RANGE_SIZE_KHR];
    VkSurfaceFormatKHR surfFormats[VK_MAX_SURFACE_FORMATS];
} vkDeviceCaps_t;

VkResult deviceSetup(void* context,
                     vklDeviceSetup_t* conf,
                     const vklDeviceInfo_t* devices,
                     uint32_t numDevices)
{
    conf->numQueues = 1;
    conf->queues = calloc(conf->numQueues, sizeof(VkDeviceQueueCreateInfo));
    for (uint32_t i = 0; i < numDevices; i++)
    {
        const vklDeviceInfo_t* info = &devices[i];
        if (info->properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            continue;
        }
        for (uint32_t j = 0; j < info->numFamilies; j++)
        {
            VkBool32 canPresent = VK_FALSE;
            vkDeviceCaps_t* caps = (vkDeviceCaps_t*)context;
            VkSurfaceKHR surface = caps->surface;
            VkQueueFamilyProperties* props = &info->families[j];
            vkGetPhysicalDeviceSurfaceSupportKHR(info->handle, j, surface, &canPresent);
            if ((props->queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && canPresent)
            {
				caps->numSurfFormats = VK_MAX_SURFACE_FORMATS;
				caps->numPresentModes = VK_PRESENT_MODE_RANGE_SIZE_KHR;
                assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->handle, surface, &caps->surfaceCaps) == VK_SUCCESS);
                assert(vkGetPhysicalDeviceSurfacePresentModesKHR(info->handle, surface, &caps->numPresentModes, caps->presentModes) == VK_SUCCESS);
                assert(vkGetPhysicalDeviceSurfaceFormatsKHR(info->handle, surface, &caps->numSurfFormats, caps->surfFormats) == VK_SUCCESS);
                conf->queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                conf->queues[0].queueFamilyIndex = j;
                conf->queues[0].queueCount = 1;
                conf->queues[0].pQueuePriorities = calloc(1, sizeof(float));
                conf->index = i;
                conf->gfxQueue = 0;
                conf->presentQueue = 0;
                return VK_SUCCESS;
            }
        }
    }
    return VK_NOT_READY;
}

int main(int argc, const char * argv[])
{
    vklInitialize(*argv);
    if (glfwInit())
    {
        GLFWmonitor* monitor = NULL;
        int width = 1280, height = 800;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef PROGRAM_FULLSCREEN
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);
        width = vidMode->width;
        height = vidMode->height;
#endif
        GLFWwindow* window = glfwCreateWindow(width, height, PROGRAM_NAME, monitor, NULL);
        if (window)
        {
			VkSurfaceKHR surface = vklCreateSurface(glfwGetNativeView(window));
            vkDeviceCaps_t deviceCaps = { .surface = surface };
            VkDevice device = vklCreateDevice(&deviceSetup, &deviceCaps);
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
            }
            vkDestroyDevice(device, NULL);
            vklDestroySurface(surface);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    vklShutdown();
    return 0;
}
