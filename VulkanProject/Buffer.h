// Buffer.h
#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <stdexcept>
#include "CommandPool.h"

class Buffer {
public:
    Buffer(VmaAllocator allocator,
           VkDeviceSize size,
           VkBufferUsageFlags usage,
           VmaMemoryUsage memoryUsage,
           VmaAllocationCreateFlags allocFlags = 0);

    ~Buffer();

    VkBuffer get() const;
    void* map();
    void unmap();
    void flush(VkDeviceSize size = VK_WHOLE_SIZE);
	void copyTo(CommandPool* commandPool,VkQueue queue, Buffer* dstBuffer);

private:
    VmaAllocator allocator_;
    VkBuffer buffer_;
    VmaAllocation allocation_;
    void* mappedData_ = nullptr;
    VkDeviceSize size_; 
};
