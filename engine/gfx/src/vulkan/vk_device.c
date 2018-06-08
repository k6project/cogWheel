#include "vk_context.h"

#include <stdlib.h>
#include <string.h>

static const char* VK_REQUIRED_DEVICE_EXTENSIONS[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const uint32_t VK_NUM_REQUIRED_DEVICE_EXTENSIONS = sizeof(VK_REQUIRED_DEVICE_EXTENSIONS) / sizeof(const char*);

static VkResult vklDeviceSelector(vklDeviceSetup_t* conf)
{
	conf->numQueues = 1;
	conf->queues = calloc(conf->numQueues, sizeof(VkDeviceQueueCreateInfo));
	for (uint32_t i = 0; i < gContext.numDevices; i++)
	{
		const vklDeviceInfo_t* info = &gContext.deviceInfo[i];
		if (info->properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			continue;
		}
		for (uint32_t j = 0; j < info->numFamilies; j++)
		{
			VkBool32 canPresent = VK_FALSE;
			VkSurfaceKHR surface = gDevice.surface;
			VkQueueFamilyProperties* props = &info->families[j];
			vkGetPhysicalDeviceSurfaceSupportKHR(info->handle, j, surface, &canPresent);
			uint32_t queueMask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
			if ((props->queueFlags & queueMask) == queueMask && canPresent)
			{
				gDevice.queueFamily = j;
				gDevice.memProps = info->memory;
				uint32_t count = 8;
				VkPresentModeKHR* modes = NULL;
				VkSurfaceFormatKHR* formats = NULL;
				VkSurfaceCapabilitiesKHR surfaceCaps;
				gDevice.numBuffers = GFX_NUM_FRAMEBUFFERS;
				gDevice.presentMode = VK_PRESENT_MODE_FIFO_KHR;
				VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->handle, surface, &surfaceCaps));
				gDevice.surfaceSize = surfaceCaps.currentExtent;
				gDevice.transform = surfaceCaps.currentTransform;
				gDevice.numBuffers = (surfaceCaps.minImageCount > gDevice.numBuffers) ? surfaceCaps.minImageCount : gDevice.numBuffers;
				gDevice.numBuffers = (surfaceCaps.maxImageCount < gDevice.numBuffers) ? surfaceCaps.maxImageCount : gDevice.numBuffers;
				VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(info->handle, surface, &count, NULL));
				ENSURE(formats = (VkSurfaceFormatKHR*)memStackAlloc(gDevice.memory, count * sizeof(VkSurfaceFormatKHR)));
				VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(info->handle, surface, &count, formats));
				for (uint32_t k = 0; k <= count; k++)
				{
					CHECK(k < count);
					if (formats[k].format == vklGetFormat(GFX_FORMAT_BGRA8_SRGB))
					{
						gDevice.surfFormat = formats[k];
						break;
					}
				}
				VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(info->handle, surface, &count, NULL));
				ENSURE(modes = (VkPresentModeKHR*)memStackAlloc(gDevice.memory, count * sizeof(VkPresentModeKHR)));
				VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(info->handle, surface, &count, modes));
				for (uint32_t k = 0; k < count && gDevice.presentMode != VK_PRESENT_MODE_MAILBOX_KHR; k++)
				{
					switch (gDevice.presentMode)
					{
					case VK_PRESENT_MODE_FIFO_KHR:
						if (modes[k] == VK_PRESENT_MODE_FIFO_RELAXED_KHR
							|| modes[k] == VK_PRESENT_MODE_MAILBOX_KHR)
						{
							gDevice.presentMode = modes[k];
						}
						break;
					case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
						if (modes[k] == VK_PRESENT_MODE_MAILBOX_KHR)
						{
							gDevice.presentMode = modes[k];
						}
						break;
					default:
						break;
					}
				}
				conf->queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				conf->queues[0].queueFamilyIndex = j;
				conf->queues[0].queueCount = 1;
				conf->queues[0].pQueuePriorities = calloc(1, sizeof(float));
				conf->index = i;
				return VK_SUCCESS;
			}
		}
	}
	return VK_NOT_READY;
}

gfxResult_t vklInitDevice(void* nativePtr)
{
	ENSURE(vklInitContext() == GFX_SUCCESS);
	ENSURE(gDevice.surface = vklCreateSurface(nativePtr));
	memStackInit(&gDevice.memory, GFX_SIZE_LINEAR_ALLOC);
	vklDeviceSetup_t setup;
	if (vklDeviceSelector(&setup) != VK_SUCCESS)
	{
		return GFX_ERR_UNKNOWN;
	}
	VkDeviceCreateInfo createInfo;
	VkPhysicalDevice gpu = gContext.deviceInfo[setup.index].handle;
	memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = setup.numQueues;
	createInfo.pQueueCreateInfos = setup.queues;
	createInfo.enabledExtensionCount = VK_NUM_REQUIRED_DEVICE_EXTENSIONS;
	createInfo.ppEnabledExtensionNames = VK_REQUIRED_DEVICE_EXTENSIONS;
	createInfo.ppEnabledLayerNames = gContext.requiredLayers;
	createInfo.enabledLayerCount = gContext.numRequiredLayers;
	ENSURE(vkCreateDevice(gpu, &createInfo, NULL, &gDevice.id) == VK_SUCCESS);
#define VULKAN_API_DEVICE(proc) \
    ENSURE(vk ## proc = ( PFN_vk ## proc )vkGetDeviceProcAddr( gDevice.id, "vk" #proc ));
#include "vk_proc.inl.h"
	size_t texSize = MEM_ALIGNED(sizeof(struct gfxTexture_t_)) + sizeof(struct gfxTextureImpl_t);
	memObjPoolInit(&gDevice.texturePool, texSize, 16);
	size_t bufSize = MEM_ALIGNED(sizeof(struct gfxBuffer_t_)) + sizeof(struct gfxBufferImpl_t);
	memObjPoolInit(&gDevice.bufferPool, bufSize, 16);
	CHECK(gDevice.surface && gDevice.id);
	VkSwapchainCreateInfoKHR scCreateInfo;
	VKINIT(scCreateInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
	scCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	scCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	scCreateInfo.surface = gDevice.surface;
	scCreateInfo.minImageCount = gDevice.numBuffers;
	scCreateInfo.imageFormat = gDevice.surfFormat.format;
	scCreateInfo.imageColorSpace = gDevice.surfFormat.colorSpace;
	scCreateInfo.imageExtent = gDevice.surfaceSize;
	scCreateInfo.imageArrayLayers = 1;
	scCreateInfo.presentMode = gDevice.presentMode;
	scCreateInfo.preTransform = gDevice.transform;
	scCreateInfo.clipped = VK_TRUE;
	VKCHECK(vkCreateSwapchainKHR(gDevice.id, &scCreateInfo, NULL, &gDevice.swapChain));
	VkImage images[GFX_NUM_FRAMEBUFFERS];
	VKCHECK(vkGetSwapchainImagesKHR(gDevice.id, gDevice.swapChain, &gDevice.numBuffers, images));
	for (uint32_t i = 0; i < gDevice.numBuffers; i++)
	{
		gfxTexture_t tex = vklNewTexture();
		tex->width = gDevice.surfaceSize.width;
		tex->height = gDevice.surfaceSize.height;
		tex->format = GFX_SYSTEM_BUFFER_FORMAT;
		tex->renderTarget = true;
		tex->impl.texture->image = images[i];
		VKCHECK(vklInitTexture(tex));
		gDevice.imgBuffers[i] = tex;
	}
	vkGetDeviceQueue(gDevice.id, gDevice.queueFamily, 0, &gDevice.cmdQueue);
	VkCommandPoolCreateInfo poolCreateInfo;
	VKINIT(poolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
	poolCreateInfo.queueFamilyIndex = gDevice.queueFamily;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VKCHECK(vkCreateCommandPool(gDevice.id, &poolCreateInfo, NULL, &gDevice.cmdPool));
	VkSemaphoreCreateInfo semCreateInfo;
	VKINIT(semCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
	VKCHECK(vkCreateSemaphore(gDevice.id, &semCreateInfo, NULL, &gDevice.canDraw));
	VKCHECK(vkCreateSemaphore(gDevice.id, &semCreateInfo, NULL, &gDevice.canSwap));
	VkCommandBufferAllocateInfo cbAllocInfo;
	VKINIT(cbAllocInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
	cbAllocInfo.commandPool = gDevice.cmdPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cbAllocInfo.commandBufferCount = gDevice.numBuffers;
	VKCHECK(vkAllocateCommandBuffers(gDevice.id, &cbAllocInfo, gDevice.cbPerFrame));
	gDevice.stagingBuffer = vklNewBuffer();
	gDevice.stagingBuffer->upload = true;
	gDevice.stagingBuffer->size = GFX_SIZE_STAGING_BUFFER;
	ENSURE(vklInitBuffer(gDevice.stagingBuffer) == VK_SUCCESS);
	struct gfxBufferImpl_t* buff = gDevice.stagingBuffer->impl.buffer;
	ENSURE(vkMapMemory(gDevice.id, buff->memory, 0, gDevice.stagingBuffer->size, 0, &gDevice.stagingBuffer->hostPtr) == VK_SUCCESS);
    return GFX_SUCCESS;
}

void vklDestroyDevice()
{
    VKCHECK(vkDeviceWaitIdle(gDevice.id));
    struct gfxBufferImpl_t* buff = gDevice.stagingBuffer->impl.buffer;
    vkUnmapMemory(gDevice.id, buff->memory);
    vklDestroyBuffer(gDevice.stagingBuffer);
    for (uint32_t i = 0; i < gDevice.numBuffers; i++)
    {
        vklDestroyTexture(gDevice.imgBuffers[i]);
    }
    vkDestroySemaphore(gDevice.id, gDevice.canSwap, NULL);
    vkDestroySemaphore(gDevice.id, gDevice.canDraw, NULL);
    vkDestroyCommandPool(gDevice.id, gDevice.cmdPool, NULL);
    vkDestroySwapchainKHR(gDevice.id, gDevice.swapChain, NULL);
    vkDestroyDevice(gDevice.id, NULL);
    vkDestroySurfaceKHR(gContext.instance, gDevice.surface, NULL);
    memObjPoolDestroy(&gDevice.texturePool);
    memStackDestroy(&gDevice.memory);
    gDevice.surface = VK_NULL_HANDLE;
    gDevice.id = VK_NULL_HANDLE;
    vklDestroyContext();
}
