#include "renderer.h"

#include <stdlib.h>
#include <assert.h>

#include <core/memory.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GFX_DEFAULT_NUM_BUFFERS 3
#define GFX_STAGING_BUFFER_SIZE (16u << 20)
#define GFX_LINEAR_ALLOC_CAPACITY (4u << 20)

struct gfxTextureImpl_t
{
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
};

struct gfxBufferImpl_t
{
    VkBuffer handle;
    VkDeviceMemory memory;
};

struct gfxShaderImpl_t
{
    VkShaderModule vertex;
	VkShaderModule hull;
	VkShaderModule domain;
	VkShaderModule geometry;
	VkShaderModule pixel;
	VkShaderModule compute;
};

struct gfxPipelineImpl_t
{
    VkPipeline handle;
};

struct gfxDevice_t
{
	memStackAlloc_t* memory;
	memObjPool_t* texturePool;
    memObjPool_t* bufferPool;
	VkDevice device;
	VkSurfaceKHR surface;
	VkExtent2D surfaceSize;
	VkSurfaceTransformFlagBitsKHR transform;
	VkSurfaceFormatKHR surfFormat;
	VkPresentModeKHR presentMode;
	VkPhysicalDeviceMemoryProperties memProps;
	uint32_t queueFamily;
	uint32_t numBuffers;
	uint32_t bufferIdx;
	gfxTexture_t* imgBuffers;
	gfxTexture_t backBuffer;
	VkQueue cmdQueue;
	VkCommandPool cmdPool;
	gfxBuffer_t stagingBuffer;
	VkSwapchainKHR swapChain;
	VkSemaphore canDraw;
	VkSemaphore canSwap;
	VkCommandBuffer* cmdBuffers;
	VkCommandBuffer cmdBuffer;
};

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
            gfxDevice_t gfx = (gfxDevice_t)context;
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
                assert(formats = (VkSurfaceFormatKHR*)memStackAlloc(gfx->memory, count * sizeof(VkSurfaceFormatKHR)));
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
				assert(modes = (VkPresentModeKHR*)memStackAlloc(gfx->memory, count * sizeof(VkPresentModeKHR)));
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

gfxBuffer_t gfxAllocBuffer(gfxDevice_t gfx)
{
    gfxBuffer_t buff = (gfxBuffer_t)memObjPoolGet(gfx->bufferPool);
    memset(buff, 0, memObjPoolGetStride(gfx->bufferPool));
    size_t offset = MEM_ALIGNED(sizeof(struct gfxBuffer_t));
    buff->impl.buffer = (struct gfxBufferImpl_t*)(((char*)buff) + offset);
    return buff;
}

VkResult gfxCreateBuffer(gfxDevice_t gfx, gfxBuffer_t buffer)
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
    struct gfxBufferImpl_t* buff = buffer->impl.buffer;
	if (vkCreateBuffer(gfx->device, &createInfo, NULL, &buff->handle) == VK_SUCCESS)
	{
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(gfx->device, buff->handle, &memReqs);
		assert(vklMemAlloc(gfx->device,
			&gfx->memProps,
			&memReqs,
			(buffer->upload) 
				? (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) 
				: VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&buff->memory) == VK_SUCCESS);
		buffer->ownGpuMem = true;
		return vkBindBufferMemory(gfx->device, buff->handle, buff->memory, 0);
	}
	return VK_NOT_READY;
}

void gfxDestroyBuffer(gfxDevice_t gfx, gfxBuffer_t buffer)
{
    struct gfxBufferImpl_t* buff = buffer->impl.buffer;
	vkDestroyBuffer(gfx->device, buff->handle, NULL);
	if (buffer->ownGpuMem)
	{
		vkFreeMemory(gfx->device, buff->memory, NULL);
		buffer->ownGpuMem = false;
	}
    memObjPoolPut(gfx->bufferPool, buffer);
}

gfxTexture_t gfxAllocTexture(gfxDevice_t gfx)
{
	gfxTexture_t tex = (gfxTexture_t)memObjPoolGet(gfx->texturePool);
	memset(tex, 0, memObjPoolGetStride(gfx->texturePool));
    size_t offset = MEM_ALIGNED(sizeof(struct gfxTexture_t));
    tex->impl.texture = (struct gfxTextureImpl_t*)(((char*)tex) + offset);
    return tex;
}

VkResult gfxCreateTexture(gfxDevice_t gfx, gfxTexture_t texture)
{
    assert(texture->impl.texture);
    struct gfxTextureImpl_t* tex = texture->impl.texture;
    assert(!tex->view);
    assert(texture->width >= 1);
    assert(texture->height >= 1);
    if (texture->numMips < 1 || texture->renderTarget)
    {
        texture->numMips = 1;
    }
    if (!tex->image)
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
        assert(vkCreateImage(gfx->device, &imageInfo, NULL, &tex->image) == VK_SUCCESS);
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(gfx->device, tex->image, &memReqs);
        assert(vklMemAlloc(gfx->device,
			&gfx->memProps,
            &memReqs,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &tex->memory) == VK_SUCCESS);
        assert(vkBindImageMemory(gfx->device, tex->image, tex->memory, 0) == VK_SUCCESS);
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
    viewInfo.image = tex->image;
    return vkCreateImageView(gfx->device, &viewInfo, NULL, &tex->view);
}

void gfxDestroyTexture(gfxDevice_t gfx, gfxTexture_t texture)
{
	VKCHECK(vkDeviceWaitIdle(gfx->device));
    assert(texture->impl.texture);
    struct gfxTextureImpl_t* tex = texture->impl.texture;
    vkDestroyImageView(gfx->device, tex->view, NULL);
    if (texture->ownGpuMem)
    {
		vkDestroyImage(gfx->device, tex->image, NULL);
        vkFreeMemory(gfx->device, tex->memory, NULL);
        texture->ownGpuMem = false;
    }
    tex->memory = VK_NULL_HANDLE;
    tex->view = VK_NULL_HANDLE;
    tex->image = VK_NULL_HANDLE;
    memObjPoolPut(gfx->texturePool, texture);
}

gfxDevice_t gfxAllocDevice()
{
	static struct gfxDevice_t gfx;
	return &gfx;
}

VkResult gfxCreateDevice(gfxDevice_t gfx, struct GLFWwindow* window)
{
	assert(gfx);
    memStackInit(&gfx->memory, GFX_LINEAR_ALLOC_CAPACITY);
    size_t texSize = MEM_ALIGNED(sizeof(struct gfxTexture_t)) + sizeof(struct gfxTextureImpl_t);
    memObjPoolInit(&gfx->texturePool, texSize, 16);
    size_t bufSize = MEM_ALIGNED(sizeof(struct gfxBuffer_t)) + sizeof(struct gfxBufferImpl_t);
    memObjPoolInit(&gfx->bufferPool, bufSize, 16);
	size_t imbBytes = GFX_DEFAULT_NUM_BUFFERS * sizeof(gfxTexture_t*);
    size_t cmbBytes = GFX_DEFAULT_NUM_BUFFERS * sizeof(VkCommandBuffer);
    size_t staticBytes = imbBytes + cmbBytes;
    uint8_t* staticMem = (uint8_t*)memStackAlloc(gfx->memory, staticBytes);
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
        gfxTexture_t tex = gfxAllocTexture(gfx);
        tex->width = gfx->surfaceSize.width;
        tex->height = gfx->surfaceSize.height;
        tex->format = gfx->surfFormat.format;
        tex->renderTarget = true;
        tex->impl.texture->image = images[i];
        VKCHECK(gfxCreateTexture(gfx, tex));
        gfx->imgBuffers[i] = tex;
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
    gfx->stagingBuffer = gfxAllocBuffer(gfx);
	gfx->stagingBuffer->upload = true;
	gfx->stagingBuffer->size = GFX_STAGING_BUFFER_SIZE;
	assert(gfxCreateBuffer(gfx, gfx->stagingBuffer) == VK_SUCCESS);
    struct gfxBufferImpl_t* buff = gfx->stagingBuffer->impl.buffer;
    return vkMapMemory(gfx->device,
		buff->memory,
		0, gfx->stagingBuffer->size,
		0, &gfx->stagingBuffer->hostPtr);
}

void gfxDestroyDevice(gfxDevice_t gfx)
{
    VKCHECK(vkDeviceWaitIdle(gfx->device));
    struct gfxBufferImpl_t* buff = gfx->stagingBuffer->impl.buffer;
    vkUnmapMemory(gfx->device, buff->memory);
	gfxDestroyBuffer(gfx, gfx->stagingBuffer);
	for (uint32_t i = 0; i < gfx->numBuffers; i++)
	{
		gfxDestroyTexture(gfx, gfx->imgBuffers[i]);
	}
	vkDestroySemaphore(gfx->device, gfx->canSwap, NULL);
	vkDestroySemaphore(gfx->device, gfx->canDraw, NULL);
	vkDestroyCommandPool(gfx->device, gfx->cmdPool, NULL);
	vkDestroySwapchainKHR(gfx->device, gfx->swapChain, NULL);
	vkDestroyDevice(gfx->device, NULL);
	vklDestroySurface(gfx->surface);
    memObjPoolDestroy(&gfx->texturePool);
    memStackDestroy(&gfx->memory);
	gfx->surface = NULL;
	gfx->device = NULL;
}

void gfxUpdateResources(gfxDevice_t gfx,
    gfxTexture_t* textures,
    size_t numTextures,
    gfxBuffer_t* buffers,
    size_t numBuffers)
{
    uint32_t numBarriers = 0, numRegions = 0;
    size_t availBytes = gfx->stagingBuffer->size;
    uint8_t* buff = (uint8_t*)gfx->stagingBuffer->hostPtr;
    size_t tmpBytes = numTextures * sizeof(VkImageMemoryBarrier);
    VkImageMemoryBarrier* barriers = (VkImageMemoryBarrier*)memStackAlloc(gfx->memory, tmpBytes);
    tmpBytes = numTextures * sizeof(VkBufferImageCopy);
    VkBufferImageCopy* regions = (VkBufferImageCopy*)memStackAlloc(gfx->memory, tmpBytes);
    memset(regions, 0, tmpBytes);
    for (size_t i = 0; i < numTextures; i++)
    {
        if (textures[i]->hasPendingData && textures[i]->imageDataSize <= availBytes)
        {
            assert(textures[i]->impl.texture);
            struct gfxTextureImpl_t* tex = textures[i]->impl.texture;
            VkImageMemoryBarrier* barrier = &barriers[numBarriers++];
            VKINIT(*barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
            memcpy(buff, textures[i]->imageData, textures[i]->imageDataSize);
            barrier->oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier->image = tex->image;
            barrier->srcQueueFamilyIndex = gfx->queueFamily;
            barrier->dstQueueFamilyIndex = gfx->queueFamily;
            barrier->image = tex->image;
            barrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;/*todo: depth/stencil ???*/
            barrier->subresourceRange.layerCount = 1;
            barrier->subresourceRange.levelCount = 1;
            VkBufferImageCopy* region = &regions[numRegions++];
            region->bufferOffset = buff - ((uint8_t*)gfx->stagingBuffer->hostPtr);
            region->imageExtent.width = textures[i]->width;
            region->imageExtent.height = textures[i]->height;
            region->imageExtent.depth = 1;
            region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region->imageSubresource.layerCount = 1;
            availBytes -= textures[i]->imageDataSize;
            buff += textures[i]->imageDataSize;
            textures[i]->hasPendingData = false;
        }
        else if (textures[i]->imageDataSize > availBytes)
        {
            break;
        }
    }
    /* todo: add update for buffers */
    assert(numBuffers == 0);
    if (numBarriers > 0)
    {
        VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, dFlags, 0, 0, NULL, 0, NULL, numBarriers, barriers);
    }
    for (uint32_t i = 0; i < numRegions; i++)
    {
        /* assumed:  texture barrier index matches region index */
        vkCmdCopyBufferToImage(gfx->cmdBuffer,
            gfx->stagingBuffer->impl.buffer->handle,
            barriers[i].image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &regions[i]);
    }
    memStackFree(gfx->memory, regions);
    memStackFree(gfx->memory, barriers);
}

void gfxClearRenderTarget(gfxDevice_t gfx,
    gfxTexture_t texture,
    vec4f_t color)
{
    gfxTexture_t tex = (texture) ? texture : gfx->backBuffer;
    VkImageMemoryBarrier barrier;
    VKINIT(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = gfx->queueFamily;
    barrier.dstQueueFamilyIndex = gfx->queueFamily;
    barrier.image = tex->impl.texture->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, dFlags, 0, 0, NULL, 0, NULL, 1, &barrier);
    VkClearColorValue* value = (VkClearColorValue*)color;
    vkCmdClearColorImage(gfx->cmdBuffer,
        tex->impl.texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        value,
        1, &barrier.subresourceRange);
}

void gfxBlitTexture(gfxDevice_t gfx,
    gfxTexture_t dest,
    gfxTexture_t src)
{
    src = (src) ? src : gfx->backBuffer;
    dest = (dest) ? dest : gfx->backBuffer;
    assert(src != dest);
    VkImageMemoryBarrier barriers[2];
    VKINIT(barriers[0], VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = gfx->queueFamily;
    barriers[0].dstQueueFamilyIndex = gfx->queueFamily;
    barriers[0].image = dest->impl.texture->image;
    barriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[0].subresourceRange.layerCount = 1;
    barriers[0].subresourceRange.levelCount = 1;
    VKINIT(barriers[1], VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT 
		| VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT 
		| VK_ACCESS_SHADER_WRITE_BIT;
	barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = gfx->queueFamily;
    barriers[1].dstQueueFamilyIndex = gfx->queueFamily;
    barriers[1].image = src->impl.texture->image;
    barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[1].subresourceRange.layerCount = 1;
    barriers[1].subresourceRange.levelCount = 1;
    VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TRANSFER_BIT 
		| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 
		| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, dFlags, 0, 0, NULL, 0, NULL, 2, barriers);
	VkImageBlit blit;
	memset(&blit, 0, sizeof(blit));
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.layerCount = 1;
	blit.srcOffsets[1].x = src->width;
	blit.srcOffsets[1].y = src->height;
	blit.srcOffsets[1].z = 1;
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.layerCount = 1;
	blit.dstOffsets[1].x = dest->width;
	blit.dstOffsets[1].y = dest->height;
	blit.dstOffsets[1].z = 1;
	vkCmdBlitImage(gfx->cmdBuffer, 
		src->impl.texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dest->impl.texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &blit, VK_FILTER_NEAREST);
}

void gfxBeginFrame(gfxDevice_t gfx)
{
    VKCHECK(vkQueueWaitIdle(gfx->cmdQueue));
	VKCHECK(vkAcquireNextImageKHR(gfx->device, gfx->swapChain, UINT64_MAX, gfx->canDraw, VK_NULL_HANDLE, &gfx->bufferIdx));
    gfx->backBuffer = gfx->imgBuffers[gfx->bufferIdx];
    gfx->cmdBuffer = gfx->cmdBuffers[gfx->bufferIdx];
    VkCommandBufferBeginInfo beginInfo;
    VKINIT(beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    VKCHECK(vkBeginCommandBuffer(gfx->cmdBuffer, &beginInfo));
}

void gfxEndFrame(gfxDevice_t gfx)
{
    VkImageMemoryBarrier barrier;
    VKINIT(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcQueueFamilyIndex = gfx->queueFamily;
    barrier.dstQueueFamilyIndex = gfx->queueFamily;
    barrier.image = gfx->backBuffer->impl.texture->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    vkCmdPipelineBarrier(gfx->cmdBuffer, sFlags, dFlags, 0, 0, NULL, 0, NULL, 1, &barrier);
    VKCHECK(vkEndCommandBuffer(gfx->cmdBuffer));
    VkSubmitInfo submitInfo;
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
