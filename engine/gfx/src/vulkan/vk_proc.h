#pragma once

#ifdef _cplusplus
extern "C"
{
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VULKAN_API_GOBAL(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_INSTANCE(proc) extern PFN_vk ## proc vk ## proc;
#define VULKAN_API_DEVICE(proc) extern PFN_vk ## proc vk ## proc;
#include "vk_proc.inl.h"

#ifdef _cplusplus
}
#endif
