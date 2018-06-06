#include "vk_proc.h"

#define VULKAN_API_GOBAL(proc) PFN_vk ## proc vk ## proc = NULL;
#define VULKAN_API_INSTANCE(proc) PFN_vk ## proc vk ## proc = NULL;
#define VULKAN_API_DEVICE(proc) PFN_vk ## proc vk ## proc = NULL;
#include "vk_proc.inl.h"
