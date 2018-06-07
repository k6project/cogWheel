#include "vk_context.h"

void vklBeginFrame()
{
	VKCHECK(vkQueueWaitIdle(gDevice.cmdQueue));
	VkResult res = vkAcquireNextImageKHR(gDevice.id,
		gDevice.swapChain, UINT64_MAX, gDevice.canDraw, VK_NULL_HANDLE, &gDevice.bufferIdx);
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		return;
	}
	VKCHECK(res);
	gDevice.backBuffer = gDevice.imgBuffers[gDevice.bufferIdx];
	gDevice.cbDraw = gDevice.cbPerFrame[gDevice.bufferIdx];
	gDevice.cbTransfer = gDevice.cbPerFrame[gDevice.bufferIdx];
	VkCommandBufferBeginInfo beginInfo;
	VKINIT(beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VKCHECK(vkBeginCommandBuffer(gDevice.cbDraw, &beginInfo));
}

void vklUpdate(gfxTexture_t* textures, size_t numTextures, gfxBuffer_t* buffers, size_t numBuffers)
{
	uint32_t numBarriers = 0, numRegions = 0;
	size_t availBytes = gDevice.stagingBuffer->size;
	uint8_t* buff = (uint8_t*)gDevice.stagingBuffer->hostPtr;
	size_t tmpBytes = numTextures * sizeof(VkImageMemoryBarrier);
	VkImageMemoryBarrier* barriers = (VkImageMemoryBarrier*)memStackAlloc(gDevice.memory, tmpBytes);
	tmpBytes = numTextures * sizeof(VkBufferImageCopy);
	VkBufferImageCopy* regions = (VkBufferImageCopy*)memStackAlloc(gDevice.memory, tmpBytes);
	memset(regions, 0, tmpBytes);
	for (size_t i = 0; i < numTextures; i++)
	{
		if (textures[i]->hasPendingData && textures[i]->imageDataSize <= availBytes)
		{
			ENSURE(textures[i]->impl.texture);
			struct gfxTextureImpl_t* tex = textures[i]->impl.texture;
			VkImageMemoryBarrier* barrier = &barriers[numBarriers++];
			VKINIT(*barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
			memcpy(buff, textures[i]->imageData, textures[i]->imageDataSize);
			barrier->oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier->newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier->image = tex->image;
			barrier->srcQueueFamilyIndex = gDevice.queueFamily;
			barrier->dstQueueFamilyIndex = gDevice.queueFamily;
			barrier->image = tex->image;
			barrier->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;/*todo: depth/stencil ???*/
			barrier->subresourceRange.layerCount = 1;
			barrier->subresourceRange.levelCount = 1;
			VkBufferImageCopy* region = &regions[numRegions++];
			region->bufferOffset = buff - ((uint8_t*)gDevice.stagingBuffer->hostPtr);
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
	ENSURE(numBuffers == 0);
	if (numBarriers > 0)
	{
		VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		vkCmdPipelineBarrier(gDevice.cbTransfer, sFlags, dFlags, 0, 0, NULL, 0, NULL, numBarriers, barriers);
	}
	for (uint32_t i = 0; i < numRegions; i++)
	{
		/* assumed:  texture barrier index matches region index */
		vkCmdCopyBufferToImage(gDevice.cbTransfer,
			gDevice.stagingBuffer->impl.buffer->handle,
			barriers[i].image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &regions[i]);
	}
	memStackFree(gDevice.memory, regions);
	memStackFree(gDevice.memory, barriers);
}

void vklClear(gfxTexture_t texture, vec4f_t color)
{
	gfxTexture_t tex = (texture) ? texture : gDevice.backBuffer;
	VkImageMemoryBarrier barrier;
	VKINIT(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = gDevice.queueFamily;
	barrier.dstQueueFamilyIndex = gDevice.queueFamily;
	barrier.image = tex->impl.texture->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	vkCmdPipelineBarrier(gDevice.cbDraw, sFlags, dFlags, 0, 0, NULL, 0, NULL, 1, &barrier);
	VkClearColorValue* value = (VkClearColorValue*)color;
	vkCmdClearColorImage(gDevice.cbDraw,
		tex->impl.texture->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		value,
		1, &barrier.subresourceRange);
}

void vklBlit(gfxTexture_t dest, gfxTexture_t src)
{
	src = (src) ? src : gDevice.backBuffer;
	dest = (dest) ? dest : gDevice.backBuffer;
	ENSURE(src != dest);
	VkImageMemoryBarrier barriers[2];
	VKINIT(barriers[0], VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barriers[0].srcQueueFamilyIndex = gDevice.queueFamily;
	barriers[0].dstQueueFamilyIndex = gDevice.queueFamily;
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
	barriers[1].srcQueueFamilyIndex = gDevice.queueFamily;
	barriers[1].dstQueueFamilyIndex = gDevice.queueFamily;
	barriers[1].image = src->impl.texture->image;
	barriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barriers[1].subresourceRange.layerCount = 1;
	barriers[1].subresourceRange.levelCount = 1;
	VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_TRANSFER_BIT
		| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	vkCmdPipelineBarrier(gDevice.cbDraw, sFlags, dFlags, 0, 0, NULL, 0, NULL, 2, barriers);
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
	vkCmdBlitImage(gDevice.cbDraw,
		src->impl.texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dest->impl.texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &blit, VK_FILTER_NEAREST);
}

void vklEndFrame()
{
	VkImageMemoryBarrier barrier;
	VKINIT(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = gDevice.queueFamily;
	barrier.dstQueueFamilyIndex = gDevice.queueFamily;
	barrier.image = gDevice.backBuffer->impl.texture->image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	VkPipelineStageFlags sFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	vkCmdPipelineBarrier(gDevice.cbDraw, sFlags, dFlags, 0, 0, NULL, 0, NULL, 1, &barrier);
	VKCHECK(vkEndCommandBuffer(gDevice.cbDraw));
	VkSubmitInfo submitInfo;
	VKINIT(submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &gDevice.cbDraw;
	submitInfo.pSignalSemaphores = &gDevice.canSwap;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &gDevice.canDraw;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitDstStageMask = &sFlags;
	VKCHECK(vkQueueSubmit(gDevice.cmdQueue, 1, &submitInfo, VK_NULL_HANDLE));
	VkPresentInfoKHR presentInfo;
	VKINIT(presentInfo, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
	presentInfo.pSwapchains = &gDevice.swapChain;
	presentInfo.pImageIndices = &gDevice.bufferIdx;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &gDevice.canSwap;
	presentInfo.waitSemaphoreCount = 1;
	VKCHECK(vkQueuePresentKHR(gDevice.cmdQueue, &presentInfo));
}
