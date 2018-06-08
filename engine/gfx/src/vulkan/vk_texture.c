#include "vk_context.h"

#include <string.h>

#include <core/memory.h>

VkFormat vklGetFormat(gfxDataFormat_t format)
{
	static const VkFormat formats[] =
	{
		/*GFX_FORMAT_BRGA8     */ VK_FORMAT_B8G8R8A8_UNORM,
		/*GFX_FORMAT_BGRA8_SRGB*/ VK_FORMAT_B8G8R8A8_SRGB,
		/*GFX_FORMAT_RGBA8     */ VK_FORMAT_R8G8B8A8_UNORM,
		/*GFX_FORMAT_RGBA8_SRGB*/ VK_FORMAT_R8G8B8A8_SRGB,
		/*GFX_FORMAT_D24S8     */ VK_FORMAT_D24_UNORM_S8_UINT,
		/*GFX_FORMAT_GRAYSCALE */ VK_FORMAT_R8_UNORM
	};
	return (format < GFX_SYSTEM_BUFFER_FORMAT) ? formats[format] : gDevice.surfFormat.format;
}

gfxTexture_t vklNewTexture()
{
    gfxTexture_t tex = (gfxTexture_t)memObjPoolGet(gDevice.texturePool);
    memset(tex, 0, memObjPoolGetStride(gDevice.texturePool));
    size_t offset = MEM_ALIGNED(sizeof(struct gfxTexture_t_));
    tex->impl.texture = (struct gfxTextureImpl_t*)(((char*)tex) + offset);
    return tex;
}

gfxResult_t vklInitTexture(gfxTexture_t texture)
{
    CHECK(texture->impl.texture);
    struct gfxTextureImpl_t* tex = texture->impl.texture;
    CHECK(!tex->view);
    CHECK(texture->width >= 1);
    CHECK(texture->height >= 1);
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
        imageInfo.format = vklGetFormat(texture->format);
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
        VKCHECK(vkCreateImage(gDevice.id, &imageInfo, NULL, &tex->image));
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(gDevice.id, tex->image, &memReqs);
        ENSURE((tex->memory = vklMemAlloc(&memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) != VK_NULL_HANDLE);
        VKCHECK(vkBindImageMemory(gDevice.id, tex->image, tex->memory, 0));
        texture->ownGpuMem = true;
    }
    VkImageViewCreateInfo viewInfo;
    VKINIT(viewInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = vklGetFormat(texture->format);
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
    VKCHECK(vkCreateImageView(gDevice.id, &viewInfo, NULL, &tex->view));
    return GFX_SUCCESS;
}

void vklDestroyTexture(gfxTexture_t texture)
{
    VKCHECK(vkDeviceWaitIdle(gDevice.id));
    CHECK(texture->impl.texture);
    struct gfxTextureImpl_t* tex = texture->impl.texture;
    vkDestroyImageView(gDevice.id, tex->view, NULL);
    if (texture->ownGpuMem)
    {
        vkDestroyImage(gDevice.id, tex->image, NULL);
        vkFreeMemory(gDevice.id, tex->memory, NULL);
        texture->ownGpuMem = false;
    }
    tex->memory = VK_NULL_HANDLE;
    tex->view = VK_NULL_HANDLE;
    tex->image = VK_NULL_HANDLE;
    memObjPoolPut(gDevice.texturePool, texture);
}
