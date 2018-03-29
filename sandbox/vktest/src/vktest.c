#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

typedef struct
{
    void* dll;
    uint32_t numLayers;
    VkLayerProperties* layers;
} gfxContextVK_t;

static inline void vkUnloadDll(gfxContextVK_t* vk)
{
    if (vk->dll)
    {
        dlclose(vk->dll);
    }
}

static inline void vkLoadDll(gfxContextVK_t* vk, const char* argv0)
{
    const char* term = strrchr(argv0, '/');
    static const char libName[] = "libvulkan.dylib";
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
    gfxContextVK_t vkApi;
    vkLoadDll(&vkApi, argv[0]);
    
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = NULL;
    vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)dlsym(vkApi.dll, "vkEnumerateInstanceLayerProperties");
    assert(vkEnumerateInstanceLayerProperties);
    
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vkApi.dll, "vkGetInstanceProcAddr");
    assert(vkGetInstanceProcAddr);
    
    PFN_vkCreateInstance vkCreateInstance = NULL;
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
    assert(vkCreateInstance);
    
    VkResult result;
    VkInstance instance;
    VkInstanceCreateInfo info;
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
    memset(&info, 0, sizeof(VkInstanceCreateInfo));
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    const char * layers[] = { "VK_LAYER_LUNARG_standard_validation" };
    info.enabledLayerCount = 1;
    info.ppEnabledLayerNames = layers;
    result = vkCreateInstance(&info, NULL, &instance);
    assert(result == VK_SUCCESS);
    PFN_vkDestroyInstance vkDestroyInstance = 0;
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(instance, "vkDestroyInstance");
    assert(vkDestroyInstance);
    vkDestroyInstance(instance, NULL);

    vkUnloadDll(&vkApi);
    return 0;
}
