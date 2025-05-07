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

    void createImage(
        uint32_t width, uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VmaMemoryUsage memoryUsage);

    void createImage(
        uint32_t width, uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkImageCreateFlags flags,
		size_t layerCount,
        VmaMemoryUsage memoryUsage
        );


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

	void setImageLayout(VkImageLayout layout) { m_ImageLayout = layout; }
	VkImageLayout getImageLayout() const { return m_ImageLayout; }

	uint32_t getWidth() const { return m_Width; }
    uint32_t getHeight() const { return m_Height; }

private:
    Device* m_pDevice;
    VmaAllocator m_Allocator;
    VkImage m_Image;
    VmaAllocation m_Allocation;
	VkImageLayout m_ImageLayout;
	uint32_t m_Width;
    uint32_t m_Height;
};
