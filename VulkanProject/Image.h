// Image.h

#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class Image {
public:
    Image(VkDevice device, VmaAllocator allocator);
    ~Image();

    void createImage(uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VmaMemoryUsage memoryUsage);

    VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags);

    void transitionImageLayout(VkCommandPool commandPool, VkQueue graphicsQueue,
                               VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    VkImage getImage() const;
    VmaAllocation getAllocation() const;

private:
    VkDevice device_;
    VmaAllocator allocator_;
    VkImage image_;
    VmaAllocation allocation_;
};
