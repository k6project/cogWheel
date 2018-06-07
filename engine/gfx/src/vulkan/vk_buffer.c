#include "vk_context.h"

#include <string.h>

#include <core/memory.h>

gfxBuffer_t vklNewBuffer()
{
    gfxBuffer_t buff = (gfxBuffer_t)memObjPoolGet(gDevice.bufferPool);
    memset(buff, 0, memObjPoolGetStride(gDevice.bufferPool));
    size_t offset = MEM_ALIGNED(sizeof(struct gfxBuffer_t_));
    buff->impl.buffer = (struct gfxBufferImpl_t*)(((char*)buff) + offset);
    return buff;
}

gfxResult_t vklInitBuffer(gfxBuffer_t buffer)
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
    if (vkCreateBuffer(gDevice.id, &createInfo, NULL, &buff->handle) == VK_SUCCESS)
    {
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(gDevice.id, buff->handle, &memReqs);
        VkMemoryPropertyFlags memFlags = (buffer->upload)
            ? (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        ENSURE((buff->memory = vklMemAlloc(&memReqs, memFlags)) != VK_NULL_HANDLE);
        buffer->ownGpuMem = true;
        VKCHECK(vkBindBufferMemory(gDevice.id, buff->handle, buff->memory, 0));
        return GFX_SUCCESS;
    }
    return GFX_ERR_UNKNOWN;
}

void vklDestroyBuffer(gfxBuffer_t buffer)
{
    struct gfxBufferImpl_t* buff = buffer->impl.buffer;
    vkDestroyBuffer(gDevice.id, buff->handle, NULL);
    if (buffer->ownGpuMem)
    {
        vkFreeMemory(gDevice.id, buff->memory, NULL);
        buffer->ownGpuMem = false;
    }
    memObjPoolPut(gDevice.bufferPool, buffer);
}
