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

    void transitionImageLayout(CommandPool* commandPool, VkQueue graphicsQueue,
                               VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void copyBufferToImage(CommandPool* commandPool, VkBuffer buffer, uint32_t width, uint32_t height);

    VkImage getImage() const;
    VmaAllocation getAllocation() const;

	bool hasStencilComponent(VkFormat format) const
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

private:
    Device* m_pDevice;
    VmaAllocator m_Allocator;
    VkImage m_Image;
    VmaAllocation m_Allocation;
};
