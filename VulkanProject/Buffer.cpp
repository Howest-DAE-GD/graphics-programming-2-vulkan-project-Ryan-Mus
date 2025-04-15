// Buffer.cpp
#include "Buffer.h"

Buffer::Buffer(VmaAllocator allocator,
               VkDeviceSize size,
               VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage,
               VmaAllocationCreateFlags allocFlags)
    : allocator_(allocator), size_(size) {

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = allocFlags;

    if (vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &buffer_, &allocation_, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }
}

Buffer::~Buffer() {
    if (mappedData_) {
        unmap();
    }
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
}

VkBuffer Buffer::get() const {
    return buffer_;
}

void* Buffer::map() {
    if (!mappedData_) {
        if (vmaMapMemory(allocator_, allocation_, &mappedData_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map buffer memory!");
        }
    }
    return mappedData_;
}

void Buffer::unmap() {
    if (mappedData_) {
        vmaUnmapMemory(allocator_, allocation_);
        mappedData_ = nullptr;
    }
}

void Buffer::flush(VkDeviceSize size) {
    vmaFlushAllocation(allocator_, allocation_, 0, size);
}

void Buffer::copyTo(CommandPool* commandPool,VkQueue queue, Buffer* dstBuffer)
{
	VkCommandBuffer commandBuffer = commandPool->beginSingleTimeCommands();
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size_;
	vkCmdCopyBuffer(commandBuffer, buffer_, dstBuffer->get(), 1, &copyRegion);
	commandPool->endSingleTimeCommands(commandBuffer, queue);
}
