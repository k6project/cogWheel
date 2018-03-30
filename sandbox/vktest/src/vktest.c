#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <vulkan/vulkan_api.h>

PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
#define VULKAN_API_GOBAL(proc) PFN_vk ## proc vk ## proc = NULL;
#define VULKAN_API_INSTANCE(proc) PFN_vk ## proc vk ## proc = NULL;
#define VULKAN_API_DEVICE(proc) PFN_vk ## proc vk ## proc = NULL;
#include <vkproc.inl.h>

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

static const uint32_t VK_NUM_REQUIRED_LAYERS = sizeof(VK_REQUIRED_LAYERS) / sizeof(const char*);
static const uint32_t VK_NUM_REQUIRED_EXTENSIONS = sizeof(VK_REQUIRED_EXTENSIONS) / sizeof(const char*);

static void initVkGlobalFunctions()
{
#define VULKAN_API_GOBAL(proc) \
    assert(vk ## proc = ( PFN_vk ## proc )vkGetInstanceProcAddr( NULL, "vk" #proc ));
#include <vkproc.inl.h>
}

static void initVkInstanceFunctions(VkInstance instance)
{
#define VULKAN_API_INSTANCE(proc) \
    assert(vk ## proc = ( PFN_vk ## proc )vkGetInstanceProcAddr( instance, "vk" #proc ));
#include <vkproc.inl.h>
}

static inline void vkUnloadDll(vkContext_t* vk)
{
    if (vk->dll)
    {
        dlclose(vk->dll);
    }
    free(vk->deviceInfo);
    free(vk->extensions);
    free(vk->layers);
    free(vk->devices);
}

static inline void vkLoadDll(vkContext_t* vk, const char* argv0)
{
    const char* term = strrchr(argv0, '/');
    static const char libName[] = "libvulkan.dylib";
    memset(vk, 0, sizeof(vkContext_t));
    if (term)
    {
        int length = term - argv0 + 1;
        char* cwd = (char*)malloc(length + sizeof(libName));
        memcpy(cwd, argv0, length);
        memcpy(cwd + length, libName, sizeof(libName));
        vk->dll = dlopen(cwd, RTLD_LOCAL | RTLD_NOW);
    }
    else
    {
        vk->dll = dlopen(libName, RTLD_LOCAL | RTLD_NOW);
    }
    if (!vk->dll)
    {
        printf("%s\n", dlerror());
        assert(0);
    }
}

int main(int argc, const char * argv[])
{
    VkResult result;
    vkContext_t vkApi;
    
    vkLoadDll(&vkApi, argv[0]);
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vkApi.dll, "vkGetInstanceProcAddr");
    assert(vkGetInstanceProcAddr);
    initVkGlobalFunctions();
    
    result = vkEnumerateInstanceLayerProperties(&vkApi.numLayers, NULL);
    assert(result == VK_SUCCESS);
    if (vkApi.numLayers > 0)
    {
        vkApi.layers = (VkLayerProperties*)malloc(vkApi.numLayers * sizeof(VkLayerProperties));
        result = vkEnumerateInstanceLayerProperties(&vkApi.numLayers, vkApi.layers);
        assert(result == VK_SUCCESS);
        for (uint32_t i = 0; i < vkApi.numLayers; i++)
        {
            printf("%s\n", vkApi.layers[i].layerName);
        }
    }
    
    result = vkEnumerateInstanceExtensionProperties(NULL, &vkApi.numExtensions, NULL);
    assert(result == VK_SUCCESS);
    if (vkApi.numExtensions > 0)
    {
        vkApi.extensions = (VkExtensionProperties*)malloc(vkApi.numExtensions * sizeof(VkExtensionProperties));
        result = vkEnumerateInstanceExtensionProperties(NULL, &vkApi.numExtensions, vkApi.extensions);
        assert(result == VK_SUCCESS);
        for (uint32_t i = 0; i < vkApi.numExtensions; i++)
        {
            printf("%s\n", vkApi.extensions[i].extensionName);
        }
    }
    
    VkApplicationInfo appInfo;
    memset(&appInfo, 0, sizeof(VkApplicationInfo));
    appInfo.sType =VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = PROGRAM_NAME;
    appInfo.applicationVersion = PROGRAM_VERSION;
    appInfo.pEngineName = ENGINE_NAME;
    appInfo.engineVersion = ENGINE_VERSION;
    appInfo.apiVersion = VK_VERSION_1_0;
    
    VkInstanceCreateInfo info;
    memset(&info, 0, sizeof(VkInstanceCreateInfo));
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &appInfo;
    info.enabledLayerCount = VK_NUM_REQUIRED_LAYERS;
    info.ppEnabledLayerNames = VK_REQUIRED_LAYERS;
    info.enabledExtensionCount = VK_NUM_REQUIRED_EXTENSIONS;
    info.ppEnabledExtensionNames = VK_REQUIRED_EXTENSIONS;
    
    VkInstance instance;
    result = vkCreateInstance(&info, NULL, &instance);
    assert(result == VK_SUCCESS);
    initVkInstanceFunctions(instance);
    
    result = vkEnumeratePhysicalDevices(instance, &vkApi.numDevices, NULL);
    assert(result == VK_SUCCESS);
    if (vkApi.numDevices > 0)
    {
        vkApi.devices = (VkPhysicalDevice*)malloc(vkApi.numDevices * sizeof(VkPhysicalDevice));
        vkApi.deviceInfo = (vkDeviceInfo_t*)malloc(vkApi.numDevices * sizeof(vkDeviceInfo_t));
        result = vkEnumeratePhysicalDevices(instance, &vkApi.numDevices, vkApi.devices);
        assert(result == VK_SUCCESS);
        for (uint32_t i = 0; i < vkApi.numDevices; i++)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(vkApi.devices[i], &props);
            strcpy(vkApi.deviceInfo[i].name, props.deviceName);
            vkApi.deviceInfo[i].type = props.deviceType;
        }
    }
    
    vkDestroyInstance(instance, NULL);

    vkUnloadDll(&vkApi);
    return 0;
}
