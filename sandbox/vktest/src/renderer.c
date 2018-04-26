#include "renderer.h"

#include <stdlib.h>
#include <assert.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GFX_DEFAULT_NUM_BUFFERS 3
#define GFX_STAGING_BUFFER_SIZE (16u << 20)
#define GFX_LINEAR_ALLOC_CAPACITY (4u << 20)

#define GFX_LINEAR_ALLOC_INIT(g,s) \
	do { g->linearAllocPos = g->linearAllocMark = g->linearAllocMem = (uint8_t*)malloc(s);\
         g->linearAllocMax = g->linearAllocRem = s;} while(0)

#define GFX_LINEAR_ALLOC_DESTROY(g) \
	do { free(g->linearAllocMem); \
		 g->linearAllocPos = g->linearAllocMark = g->linearAllocMem = NULL;\
		 g->linearAllocMax = g->linearAllocRem = 0;} while(0)

#define GFX_LINEAR_ALLOC_RESET(g) \
	do { g->linearAllocPos = g->linearAllocMark;\
		 g->linearAllocRem = g->linearAllocMax - (g->linearAllocMark - g->linearAllocMem);} while(0)

static void* gfxMalloc(gfxContext_t* gfx, size_t size)
{
	void* result = NULL;
	if (gfx->linearAllocRem >= size)
	{
		result = gfx->linearAllocPos;
		gfx->linearAllocPos += size;
		gfx->linearAllocRem -= size;
	}
    return result;
}

static void* gfxMallocStatic(gfxContext_t* gfx, size_t size)
{
	void* result = NULL;
	if (gfx->linearAllocPos == gfx->linearAllocMark)
	{
		result = gfxMalloc(gfx, size);
		if (result)
		{
			gfx->linearAllocMark = gfx->linearAllocPos;
		}
	}
	return result;
}

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
            uint32_t queueMask = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
            if ((props->queueFlags & queueMask) == queueMask && canPresent)
            {
				gfx->queueFamily = j;
                gfx->memProps = info->memory;
                uint32_t count = 8;
				VkPresentModeKHR* modes = NULL;
				VkSurfaceFormatKHR* formats = NULL;
				VkSurfaceCapabilitiesKHR surfaceCaps;
				gfx->numBuffers = GFX_DEFAULT_NUM_BUFFERS;
				gfx->presentMode = VK_PRESENT_MODE_FIFO_KHR;
				VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->handle, surface, &surfaceCaps));
				gfx->surfaceSize = surfaceCaps.currentExtent;
				gfx->transform = surfaceCaps.currentTransform;
				gfx->numBuffers = (surfaceCaps.minImageCount > gfx->numBuffers) ? surfaceCaps.minImageCount : gfx->numBuffers;
				gfx->numBuffers = (surfaceCaps.maxImageCount < gfx->numBuffers) ? surfaceCaps.maxImageCount : gfx->numBuffers;
                VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(info->handle, surface, &count, NULL));
                assert(formats = (VkSurfaceFormatKHR*)gfxMalloc(gfx, count * sizeof(VkSurfaceFormatKHR)));
                VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(info->handle, surface, &count, formats));
                for (uint32_t k = 0; k <= count; k++)
                {
					assert(k < count);
                    if (formats[k].format == GFX_FORMAT_BGRA8_SRGB)
                    {
                        gfx->surfFormat = formats[k];
                        break;
                    }
                }
				VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(info->handle, surface, &count, NULL));
				assert(modes = (VkPresentModeKHR*)gfxMalloc(gfx, count * sizeof(VkPresentModeKHR)));
				VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(info->handle, surface, &count, modes));
				for (uint32_t k = 0; k < count && gfx->presentMode != VK_PRESENT_MODE_MAILBOX_KHR; k++)
				{
					switch (gfx->presentMode)
					{
					case VK_PRESENT_MODE_FIFO_KHR:
						if (modes[k] == VK_PRESENT_MODE_FIFO_RELAXED_KHR
							|| modes[k] == VK_PRESENT_MODE_MAILBOX_KHR)
						{
							gfx->presentMode = modes[k];
						}
						break;
					case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
						if (modes[k] == VK_PRESENT_MODE_MAILBOX_KHR)
						{
							gfx->presentMode = modes[k];
						}
						break;
					}
				}
                /*
                 vkGetPhysicalDeviceSurfaceCapabilitiesKHR(info->handle, surface, &caps->surfaceCaps);*/
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
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (texture->renderTarget)
        {
            imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
	VKINIT(viewInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
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
    if (texture->ownGpuMem)
    {
		vkDestroyImage(gfx->device, texture->image, NULL);
        vkFreeMemory(gfx->device, texture->memory, NULL);
        texture->ownGpuMem = false;
    }
    texture->memory = VK_NULL_HANDLE;
    texture->handle = VK_NULL_HANDLE;
    texture->image = VK_NULL_HANDLE;
}

VkResult gfxCreateDevice(gfxContext_t* gfx, GLFWwindow* window)
{
	GFX_LINEAR_ALLOC_INIT(gfx, GFX_LINEAR_ALLOC_CAPACITY);
	size_t imbBytes = GFX_DEFAULT_NUM_BUFFERS * sizeof(gfxTexture_t);
    size_t cmbBytes = GFX_DEFAULT_NUM_BUFFERS * sizeof(VkCommandBuffer);
    size_t staticBytes = imbBytes + cmbBytes;
    uint8_t* staticMem = (uint8_t*)gfxMallocStatic(gfx, staticBytes);
    memset(staticMem, 0, staticBytes);
	gfx->imgBuffers = (gfxTexture_t*)staticMem;
    gfx->cmdBuffers = (VkCommandBuffer*)(staticMem + imbBytes);
	gfx->surface = vklCreateSurface(glfwGetNativeView(window));
	gfx->device = vklCreateDevice(&gfxDeviceSetupCallback, gfx);
	assert(gfx->surface && gfx->device);
	VkSwapchainCreateInfoKHR scCreateInfo;
	VKINIT(scCreateInfo, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
	scCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	scCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	scCreateInfo.surface = gfx->surface;
	scCreateInfo.minImageCount = gfx->numBuffers;
	scCreateInfo.imageFormat = gfx->surfFormat.format;
	scCreateInfo.imageColorSpace = gfx->surfFormat.colorSpace;
	scCreateInfo.imageExtent = gfx->surfaceSize;
	scCreateInfo.imageArrayLayers = 1;
	scCreateInfo.presentMode = gfx->presentMode;
	scCreateInfo.preTransform = gfx->transform;
	scCreateInfo.clipped = VK_TRUE;
	VKCHECK(vkCreateSwapchainKHR(gfx->device, &scCreateInfo, NULL, &gfx->swapChain));
	VkImage images[GFX_DEFAULT_NUM_BUFFERS];
	VKCHECK(vkGetSwapchainImagesKHR(gfx->device, gfx->swapChain, &gfx->numBuffers, images));
	for (uint32_t i = 0; i < gfx->numBuffers; i++)
	{
		gfx->imgBuffers[i].width = gfx->surfaceSize.width;
		gfx->imgBuffers[i].height = gfx->surfaceSize.height;
		gfx->imgBuffers[i].format = gfx->surfFormat.format;
		gfx->imgBuffers[i].renderTarget = true;
		gfx->imgBuffers[i].image = images[i];
		VKCHECK(gfxCreateTexture(gfx, &gfx->imgBuffers[i]));
	}
	vkGetDeviceQueue(gfx->device, gfx->queueFamily, 0, &gfx->cmdQueue);
	VkCommandPoolCreateInfo poolCreateInfo;
	VKINIT(poolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
	poolCreateInfo.queueFamilyIndex = gfx->queueFamily;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VKCHECK(vkCreateCommandPool(gfx->device, &poolCreateInfo, NULL, &gfx->cmdPool));
	VkSemaphoreCreateInfo semCreateInfo;
	VKINIT(semCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
	VKCHECK(vkCreateSemaphore(gfx->device, &semCreateInfo, NULL, &gfx->canDraw));
	VKCHECK(vkCreateSemaphore(gfx->device, &semCreateInfo, NULL, &gfx->canSwap));
    VkCommandBufferAllocateInfo cbAllocInfo;
    VKINIT(cbAllocInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    cbAllocInfo.commandPool = gfx->cmdPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = gfx->numBuffers;
    VKCHECK(vkAllocateCommandBuffers(gfx->device, &cbAllocInfo, gfx->cmdBuffers));
	gfx->stagingBuffer.upload = true;
	gfx->stagingBuffer.size = GFX_STAGING_BUFFER_SIZE;
	assert(gfxCreateBuffer(gfx, &gfx->stagingBuffer) == VK_SUCCESS);
	return vkMapMemory(gfx->device,
		gfx->stagingBuffer.memory,
		0,
		gfx->stagingBuffer.size,
		0,
		&gfx->stagingBuffer.hostPtr);
}

void gfxDestroyDevice(gfxContext_t* gfx)
{
    VKCHECK(vkDeviceWaitIdle(gfx->device));
    vkUnmapMemory(gfx->device, gfx->stagingBuffer.memory);
	gfxDestroyBuffer(gfx, &gfx->stagingBuffer);
	for (uint32_t i = 0; i < gfx->numBuffers; i++)
	{
		gfxDestroyTexture(gfx, &gfx->imgBuffers[i]);
	}
	vkDestroySemaphore(gfx->device, gfx->canSwap, NULL);
	vkDestroySemaphore(gfx->device, gfx->canDraw, NULL);
	vkDestroyCommandPool(gfx->device, gfx->cmdPool, NULL);
	vkDestroySwapchainKHR(gfx->device, gfx->swapChain, NULL);
	vkDestroyDevice(gfx->device, NULL);
	vklDestroySurface(gfx->surface);
	GFX_LINEAR_ALLOC_DESTROY(gfx);
	gfx->surface = NULL;
	gfx->device = NULL;
}

void gfxUpdateResources(gfxContext_t* gfx,
    gfxTexture_t* textures,
    size_t numTextures,
    gfxBuffer_t* buffers,
    size_t numBuffers)
{
    uint32_t numBarriers = 0, numRegions = 0;
    size_t availBytes = gfx->stagingBuffer.size;
    uint8_t* buff = (uint8_t*)gfx->stagingBuffer.hostPtr;
    size_t tmpBytes = numTextures * sizeof(VkImageMemoryBarrier);
    VkImageMemoryBarrier* barriers = (VkImageMemoryBarrier*)gfxMalloc(gfx, tmpBytes);
    tmpBytes = numTextures * sizeof(VkBufferImageCopy);
    VkBufferImageCopy* regions = (VkBufferImageCopy*)gfxMalloc(gfx, tmpBytes);
    memset(regions, 0, tmpBytes);
    for (size_t i = 0; i < numTextures; i++)
    {
        if (textures[i].hasPendingData && textures[i].imageDataSize <= availBytes)
        {
            VkImageMemoryBarrier* barrier = &barriers[numBarriers++];
            VKINIT(*barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
            memcpy(buff, textures[i].imageData, textures[i].imageDataSize);
            barrier->oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier->image = textures[i].image;
            barrier->srcQueueFamilyIndex = gfx->queueFamily;
            barrier->dstQueueFamilyIndex = gfx->queueFamily;
            barrier->image = textures[i].image;
            barrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;/*todo: depth/stencil ???*/
            barrier->subresourceRange.layerCount = 1;
            barrier->subresourceRange.levelCount = 1;
            VkBufferImageCopy* region = &regions[numRegions++];
            region->bufferOffset = buff - ((uint8_t*)gfx->stagingBuffer.hostPtr);
            region->bufferRowLength = textures[i].imageDataSize;
            //region->bufferImageHeight = 1;
            region->imageExtent.width = textures[i].width;
            region->imageExtent.height = textures[i].height;
            region->imageExtent.depth = 1;
            region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region->imageSubresource.layerCount = 1;
            availBytes -= textures[i].imageDataSize;
            buff += textures[i].imageDataSize;
            textures[i].hasPendingData = false;
        }
        else if (textures[i].imageDataSize > availBytes)
        {
            break;
        }
    }
    /* todo: add update for buffers */
    assert(numBuffers == 0);
    if (numBarriers > 0)
    {
        VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, sFlags, 0, 0, NULL, 0, NULL, numBarriers, barriers);
    }
    for (uint32_t i = 0; i < numRegions; i++)
    {
        /* assumed:  texture barrier index matches region index */
        vkCmdCopyBufferToImage(gfx->cmdBuffer,
            gfx->stagingBuffer.handle,
            barriers[i].image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &regions[i]);
    }
}

void gfxClearRenderTarget(gfxContext_t* gfx,
    gfxTexture_t* texture,
    vec4f_t color)
{
    gfxTexture_t* tex = (texture) ? texture : gfx->backBuffer;
    VkImageMemoryBarrier barrier;
    VKINIT(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = gfx->queueFamily;
    barrier.dstQueueFamilyIndex = gfx->queueFamily;
    barrier.image = tex->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, sFlags, 0, 0, NULL, 0, NULL, 1, &barrier);
    VkClearColorValue* value = (VkClearColorValue*)color;
    vkCmdClearColorImage(gfx->cmdBuffer,
        tex->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        value,
        1,
        &barrier.subresourceRange);
}

void gfxBlitTexture(gfxContext_t* gfx,
    gfxTexture_t* dest,
    gfxTexture_t* src)
{
    src = (src) ? src : gfx->backBuffer;
    dest = (dest) ? dest : gfx->backBuffer;
    assert(src != dest);
    VkImageMemoryBarrier barriers[2];
    VKINIT(barriers[0], VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = gfx->queueFamily;
    barriers[0].dstQueueFamilyIndex = gfx->queueFamily;
    barriers[0].image = dest->image;
    barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[0].subresourceRange.layerCount = 1;
    barriers[0].subresourceRange.levelCount = 1;
    VKINIT(barriers[1], VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = gfx->queueFamily;
    barriers[1].dstQueueFamilyIndex = gfx->queueFamily;
    barriers[1].image = src->image;
    barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[1].subresourceRange.layerCount = 1;
    barriers[1].subresourceRange.levelCount = 1;
    VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, sFlags, 0, 0, NULL, 0, NULL, 2, barriers);
}

void gfxBeginFrame(gfxContext_t* gfx)
{
	GFX_LINEAR_ALLOC_RESET(gfx);
    VKCHECK(vkQueueWaitIdle(gfx->cmdQueue));
	VKCHECK(vkAcquireNextImageKHR(gfx->device, gfx->swapChain, UINT64_MAX, gfx->canDraw, VK_NULL_HANDLE, &gfx->bufferIdx));
    gfx->backBuffer = &gfx->imgBuffers[gfx->bufferIdx];
    gfx->cmdBuffer = gfx->cmdBuffers[gfx->bufferIdx];
    VkCommandBufferBeginInfo beginInfo;
    VKINIT(beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    VKCHECK(vkBeginCommandBuffer(gfx->cmdBuffer, &beginInfo));
}

void gfxEndFrame(gfxContext_t* gfx)
{
    VkImageMemoryBarrier barrier;
    VKINIT(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcQueueFamilyIndex = gfx->queueFamily;
    barrier.dstQueueFamilyIndex = gfx->queueFamily;
    barrier.image = gfx->backBuffer->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, sFlags, 0, 0, NULL, 0, NULL, 1, &barrier);
    VKCHECK(vkEndCommandBuffer(gfx->cmdBuffer));
    VkSubmitInfo submitInfo;
    sFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VKINIT(submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &gfx->cmdBuffer;
    submitInfo.pSignalSemaphores = &gfx->canSwap;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &gfx->canDraw;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &sFlags;
    VKCHECK(vkQueueSubmit(gfx->cmdQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VkPresentInfoKHR presentInfo;
	VKINIT(presentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
	presentInfo.pSwapchains = &gfx->swapChain;
	presentInfo.pImageIndices = &gfx->bufferIdx;
	presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &gfx->canSwap;
    presentInfo.waitSemaphoreCount = 1;
	VKCHECK(vkQueuePresentKHR(gfx->cmdQueue, &presentInfo));
}
