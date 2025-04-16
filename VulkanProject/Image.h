// Image.h
#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class Device;
class CommandPool;
class Image 
{
public:
    Image(Device* pDevice, VmaAllocator allocator);
    ~Image();

    void createImage(uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);

    VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags);

    void transitionImageLayout(VkCommandPool commandPool, VkQueue graphicsQueue,
                               VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void copyBufferToImage(CommandPool* commandPool, VkBuffer buffer, uint32_t width, uint32_t height);

    VkImage getImage() const;
    VmaAllocation getAllocation() const;

private:
    Device* m_pDevice;
    VmaAllocator m_Allocator;
    VkImage m_Image;
    VmaAllocation m_Allocation;
};
