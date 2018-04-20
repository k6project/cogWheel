#include <stdio.h>
#include <stdbool.h>
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

/* TEST APPLICATION CODE */

#define VK_MAX_SURFACE_FORMATS 8u

#define SHARED_MEM_BUDGET 0x04000000ul /* 64MB  */
#define GPU_MEM_BUDGET    0x0C000000ul /* 192MB */

#define SHARED_MEM_MASK (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define GPU_MEM_MASK (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

/*
 
 vulkan10.h - loader and helper functions for vulkan library
 vulkan10.c
 
 renderer.h
 renderer.c - application-specific implementation (can be copied over)
 
 spirvlib.h
 spirvlib.c - app-specific shaders serialized as arrays
 
 frame flow: if upload job is staged, perform it, otherwise draw graphics
             or use parallel queues to submit both
 
 initial values for memory budgets: 64 mb staging (cpu coherent), 192 mb device local

 */

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
	bool renderTarget	  : 1;
	bool sampledTexture	  : 1;
	bool ownGpuMem        : 1;
	bool hasPendingData   : 1;
	void* imageData;
	size_t imageDataSize;
} gfxTexture_t;

VkResult gfxDeviceSetupCallback(void* context,
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
			/*vkDeviceCaps_t* caps = (vkDeviceCaps_t*)context;*/
			gfxContext_t* gfx = (gfxContext_t*)context;
			VkSurfaceKHR surface = gfx->surface;
			VkQueueFamilyProperties* props = &info->families[j];
			vkGetPhysicalDeviceSurfaceSupportKHR(info->handle, j, surface, &canPresent);
			if ((props->queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && canPresent)
			{
				gfx->memProps = info->memory;
				/*caps->numSurfFormats = VK_MAX_SURFACE_FORMATS;
				caps->numPresentModes = VK_PRESENT_MODE_RANGE_SIZE_KHR;
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->handle, surface, &caps->surfaceCaps);
				vkGetPhysicalDeviceSurfacePresentModesKHR(info->handle, surface, &caps->numPresentModes, caps->presentModes);
				vkGetPhysicalDeviceSurfaceFormatsKHR(info->handle, surface, &caps->numSurfFormats, caps->surfFormats);*/
				/* TODO: decide on surface format, presentation mode and number of buffers in swapchain */
				/* TODO: decide on memory types according to requirements */
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

VkResult gfxCreateTexture(gfxContext_t* gfx, gfxTexture_t* texture)
{
	assert(!texture->handle);
	assert(texture->width >= 1);
	assert(texture->height >= 1);
	if (texture->numMips < 1 || texture->renderTarget)
	{
		texture->numMips = 1;
	}
	if (!texture->image)
	{
		VkImageCreateInfo imageInfo;
		memset(&imageInfo, 0, sizeof(imageInfo));
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.depth = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.extent.width = texture->width;
		imageInfo.extent.height = texture->height;
		imageInfo.format = texture->format; 
		imageInfo.mipLevels = texture->numMips;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		if (texture->renderTarget)
		{
			switch (texture->format)
			{
			case GFX_FORMAT_D24S8:
				imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				break;
			default:
				imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				break;
			}
		}
		if (texture->sampledTexture)
		{
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		assert(vkCreateImage(gfx->device, &imageInfo, NULL, &texture->image) == VK_SUCCESS);
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(gfx->device, texture->image, &memReqs);
		assert(vklMemAlloc(gfx->device, 
			&gfx->memProps, 
			&memReqs, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			&texture->memory) == VK_SUCCESS);
		assert(vkBindImageMemory(gfx->device, texture->image, texture->memory, 0) == VK_SUCCESS);
		texture->ownGpuMem = true;
	}
	VkImageViewCreateInfo viewInfo;
	memset(&viewInfo, 0, sizeof(viewInfo));
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = texture->format;
	switch (texture->format)
	{
	case GFX_FORMAT_D24S8:
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	default:
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	}
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.image = texture->image;
	return vkCreateImageView(gfx->device, &viewInfo, NULL, &texture->handle);
}

void gfxDestroyTexture(gfxContext_t* gfx, gfxTexture_t* texture)
{
	vkDestroyImageView(gfx->device, texture->handle, NULL);
	vkDestroyImage(gfx->device, texture->image, NULL);
	if (texture->ownGpuMem)
	{
		vkFreeMemory(gfx->device, texture->memory, NULL);
		texture->ownGpuMem = false;
	}
	texture->handle = NULL;
	texture->image = NULL;
	texture->memory = NULL;
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
			gfxContext_t gfx;
			gfx.surface = vklCreateSurface(glfwGetNativeView(window));
			gfx.device = vklCreateDevice(&gfxDeviceSetupCallback, &gfx);

			gfxTexture_t tex;
			memset(&tex, 0, sizeof(tex));
			tex.format = GFX_DEFAULT_COLOR_FORMAT;
			tex.width = 512;
			tex.height = 512;
			tex.renderTarget = true;
			assert(gfxCreateTexture(&gfx, &tex) == VK_SUCCESS);
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
            }
			gfxDestroyTexture(&gfx, &tex);
            vkDestroyDevice(gfx.device, NULL);
            vklDestroySurface(gfx.surface);
        }
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    vklShutdown();
    return 0;
}
