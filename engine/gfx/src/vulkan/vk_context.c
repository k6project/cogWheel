#include "vk_context.h"
#include "vk_proc.h"

#include <assert.h>
#include <stdlib.h>

vklDevice_t gDevice;

vklContext_t gContext = { .dll = NULL };

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

gfxApi_t gfxApi()
{
	static struct gfxApi_t_ global =
	{
		&vklInitDevice, &vklDestroyDevice, 
		&vklNewBuffer, &vklInitBuffer, &vklDestroyBuffer,
		&vklNewTexture, &vklInitTexture, &vklDestroyTexture,
		&vklBeginFrame,
		&vklUpdate,
		&vklClear,
		&vklBlit,
		&vklEndFrame
	};
	return &global;
}

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
#ifdef DEBUG_BUILD
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

gfxResult_t vklInitContext()
{
	if (gContext.dll == NULL)
	{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		gContext.dll = LoadLibrary("vulkan-1.dll");
#else
		const char* term = strrchr(appArg, '/');
		static const char libName[] = "libvulkan.dylib";
		if (term)
		{
			int length = term - appArg + 1;
			char* cwd = (char*)malloc(length + sizeof(libName));
			memcpy(cwd, appArg, length);
			memcpy(cwd + length, libName, sizeof(libName));
			gContext.dll = dlopen(cwd, RTLD_LOCAL | RTLD_NOW);
		}
		else
		{
			gContext.dll = dlopen(libName, RTLD_LOCAL | RTLD_NOW);
		}
		if (!gContext.dll)
		{
			printf("%s\n", dlerror());
			assert(0);
		}
#endif
		VkResult result = VK_SUCCESS;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(gContext.dll, "vkGetInstanceProcAddr");
#else
		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(gContext.dll, "vkGetInstanceProcAddr");
#endif
		assert(vkGetInstanceProcAddr);
#define VULKAN_API_GOBAL(proc) \
    ENSURE(vk ## proc = ( PFN_vk ## proc )vkGetInstanceProcAddr( NULL, "vk" #proc ));
#include "vk_proc.inl.h"
		result = vkEnumerateInstanceLayerProperties(&gContext.numLayers, NULL);
		assert(result == VK_SUCCESS);
#ifdef DEBUG_BUILD
		if (gContext.numLayers > 0)
		{
			gContext.layers = (VkLayerProperties*)malloc(gContext.numLayers * sizeof(VkLayerProperties));
			result = vkEnumerateInstanceLayerProperties(&gContext.numLayers, gContext.layers);
			assert(result == VK_SUCCESS);
			for (uint32_t i = 0; i < gContext.numLayers; i++)
			{
				printf("%s\n", gContext.layers[i].layerName);
			}
		}
#endif
		result = vkEnumerateInstanceExtensionProperties(NULL, &gContext.numExtensions, NULL);
		assert(result == VK_SUCCESS);
#ifdef DEBUG_BUILD
		if (gContext.numExtensions > 0)
		{
			gContext.extensions = (VkExtensionProperties*)malloc(gContext.numExtensions * sizeof(VkExtensionProperties));
			result = vkEnumerateInstanceExtensionProperties(NULL, &gContext.numExtensions, gContext.extensions);
			assert(result == VK_SUCCESS);
			for (uint32_t i = 0; i < gContext.numExtensions; i++)
			{
				printf("%s\n", gContext.extensions[i].extensionName);
			}
		}
#endif
		VkApplicationInfo appInfo;
		memset(&appInfo, 0, sizeof(VkApplicationInfo));
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
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
		result = vkCreateInstance(&info, NULL, &gContext.instance);
		assert(result == VK_SUCCESS);
		gContext.numRequiredLayers = VK_NUM_REQUIRED_LAYERS;
		gContext.requiredLayers = VK_REQUIRED_LAYERS;
#define VULKAN_API_INSTANCE(proc) \
    ENSURE(vk ## proc = ( PFN_vk ## proc )vkGetInstanceProcAddr( gContext.instance, "vk" #proc ));
#include "vk_proc.inl.h"
		result = vkEnumeratePhysicalDevices(gContext.instance, &gContext.numDevices, NULL);
		assert(result == VK_SUCCESS);
		if (gContext.numDevices > 0)
		{
			gContext.deviceInfo = (vklDeviceInfo_t*)calloc(gContext.numDevices, sizeof(vklDeviceInfo_t));
			VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(gContext.numDevices * sizeof(VkPhysicalDevice));
			result = vkEnumeratePhysicalDevices(gContext.instance, &gContext.numDevices, devices);
			assert(result == VK_SUCCESS);
			for (uint32_t i = 0; i < gContext.numDevices; i++)
			{
				gContext.deviceInfo[i].handle = devices[i];
				vkGetPhysicalDeviceFeatures(devices[i], &gContext.deviceInfo[i].features);
				vkGetPhysicalDeviceProperties(devices[i], &gContext.deviceInfo[i].properties);
				vkGetPhysicalDeviceMemoryProperties(devices[i], &gContext.deviceInfo[i].memory);
				vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &gContext.deviceInfo[i].numFamilies, NULL);
				if (gContext.deviceInfo[i].numFamilies > 0)
				{
					uint32_t numFamilies = gContext.deviceInfo[i].numFamilies;
					gContext.deviceInfo[i].families = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * numFamilies);
					vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &gContext.deviceInfo[i].numFamilies, gContext.deviceInfo[i].families);
				}
				for (uint32_t j = 0; j < gContext.deviceInfo[i].memory.memoryHeapCount; j++)
				{
					gContext.deviceInfo[i].memTotalSize += gContext.deviceInfo[i].memory.memoryHeaps[j].size;
				}
			}
			free(devices);
		}
	}
	return GFX_SUCCESS;
}

void vklDestroyContext()
{
	if (gContext.dll)
	{
#ifdef VK_USE_PLATFORM_WIN32_KHR
		FreeLibrary(gContext.dll);
#else
		dlclose(gContext.dll);
#endif
	}
	free(gContext.deviceInfo);
	free(gContext.extensions);
	free(gContext.layers);
}
