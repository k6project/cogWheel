#include "vk_context.h"
#include "vk_proc.h"

#include <assert.h>
#include <string.h>

VkSurfaceKHR vklCreateSurface(void* nativePtr)
{
    VkResult result = VK_SUCCESS;
    VkSurfaceKHR surface = NULL;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.pNext = NULL;
    info.flags = 0;
    info.hinstance = GetModuleHandle(NULL);
    info.hwnd = nativePtr;
    result = vkCreateWin32SurfaceKHR(vkCtx_.instance, &info, NULL, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    VkMacOSSurfaceCreateInfoMVK info;
    info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    info.pNext = NULL;
    info.pView = nativePtr;
    info.flags = 0;
    result = vkCreateMacOSSurfaceMVK(gContext.instance, &info, NULL, &surface);
#endif
    assert(result == VK_SUCCESS);
    return surface;
}

VkResult vklMemAlloc(const VkMemoryRequirements* reqs, const VkMemoryPropertyFlags flags, VkDeviceMemory* memory)
{
    uint32_t type = VK_MAX_MEMORY_TYPES;
    for (uint32_t i = 0; i < gDevice.memProps.memoryTypeCount; i++)
    {
        if ((reqs->memoryTypeBits & (1 << i))
            && ((gDevice.memProps.memoryTypes[i].propertyFlags & flags) == flags))
        {
            type = i;
            break;
        }
    }
    if (type < gDevice.memProps.memoryTypeCount)
    {
        VkMemoryAllocateInfo allocInfo;
        memset(&allocInfo, 0, sizeof(allocInfo));
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = reqs->size;
        allocInfo.memoryTypeIndex = type;
        return vkAllocateMemory(gDevice.id, &allocInfo, NULL, memory);
    }
    return VK_NOT_READY;
}
