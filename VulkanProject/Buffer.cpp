// Buffer.cpp
#include "Buffer.h"
#include <spdlog/spdlog.h>

Buffer::Buffer(VmaAllocator allocator,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage,
               VmaAllocationCreateFlags allocFlags)
    : m_Allocator(allocator), m_BufferSize(size) 
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = allocFlags;

    if (vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create buffer!");
    }
	spdlog::debug("Buffer created with size: {} bytes", size);
}

Buffer::~Buffer() 
{
    if (m_pMappedData) {
        unmap();
    }
    vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
	spdlog::debug("Buffer destroyed.");
}

VkBuffer Buffer::get() const 
{
    return m_Buffer;
}

void* Buffer::map() 
{
    if (!m_pMappedData) 
    {
        if (vmaMapMemory(m_Allocator, m_Allocation, &m_pMappedData) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map buffer memory!");
        }
    }
    return m_pMappedData;
}

void Buffer::unmap() 
{
    if (m_pMappedData) 
    {
        vmaUnmapMemory(m_Allocator, m_Allocation);
        m_pMappedData = nullptr;
    }
}

void Buffer::flush(VkDeviceSize size) 
{
    vmaFlushAllocation(m_Allocator, m_Allocation, 0, size);
}

void Buffer::copyTo(CommandPool* commandPool,VkQueue queue, Buffer* dstBuffer)
{
	VkCommandBuffer commandBuffer = commandPool->beginSingleTimeCommands();
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = m_BufferSize;
	vkCmdCopyBuffer(commandBuffer, m_Buffer, dstBuffer->get(), 1, &copyRegion);
	commandPool->endSingleTimeCommands(commandBuffer, queue);
	spdlog::debug("Buffer copied to destination buffer.");
}
