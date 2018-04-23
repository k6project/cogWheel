#include "renderer.h"

#include <stdlib.h>
#include <assert.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GFX_STAGING_BUFFER_SIZE (16u << 20)

static VkResult gfxDeviceSetupCallback(void* context,
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

VkResult gfxCreateBuffer(gfxContext_t* gfx, gfxBuffer_t* buffer)
{
	VkBufferCreateInfo createInfo;
	memset(&createInfo, 0, sizeof(createInfo));
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage |= (buffer->upload) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	createInfo.usage |= (buffer->uniform) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	createInfo.usage |= (buffer->vertex) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	createInfo.usage |= (buffer->index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = buffer->size;
	if (vkCreateBuffer(gfx->device, &createInfo, NULL, &buffer->handle) == VK_SUCCESS)
	{
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(gfx->device, buffer->handle, &memReqs);
		assert(vklMemAlloc(gfx->device,
			&gfx->memProps,
			&memReqs,
			(buffer->upload) 
				? (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) 
				: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&buffer->memory) == VK_SUCCESS);
		buffer->ownGpuMem = true;
		return vkBindBufferMemory(gfx->device, buffer->handle, buffer->memory, 0);
	}
	return VK_NOT_READY;
}

void gfxDestroyBuffer(gfxContext_t* gfx, gfxBuffer_t* buffer)
{
	vkDestroyBuffer(gfx->device, buffer->handle, NULL);
	if (buffer->ownGpuMem)
	{
		vkFreeMemory(gfx->device, buffer->memory, NULL);
		buffer->ownGpuMem = false;
	}
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

VkResult gfxCreateDevice(gfxContext_t* gfx, GLFWwindow* window)
{
	gfx->surface = vklCreateSurface(glfwGetNativeView(window));
	gfx->device = vklCreateDevice(&gfxDeviceSetupCallback, gfx);
	if (gfx->surface && gfx->device)
	{
		gfx->stagingBuffer.upload = true;
		gfx->stagingBuffer.size = GFX_STAGING_BUFFER_SIZE;
		return gfxCreateBuffer(gfx, &gfx->stagingBuffer);
	}
	return VK_NOT_READY;
}

void gfxDestroyDevice(gfxContext_t* gfx)
{
	gfxDestroyBuffer(gfx, &gfx->stagingBuffer);
	vkDestroyDevice(gfx->device, NULL);
	vklDestroySurface(gfx->surface);
	gfx->surface = NULL;
	gfx->device = NULL;
}
