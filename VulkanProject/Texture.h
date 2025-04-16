#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "CommandPool.h"
#include "Image.h"
#include <string>

class Device;
class Texture {
public:
    Texture(Device* device, VmaAllocator allocator, CommandPool* commandPool,
            const std::string& texturePath, VkPhysicalDevice physicalDevice);
    ~Texture();

    void createTextureImage();
    void createTextureImageView();
    static void createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice);
    static VkSampler getTextureSampler();

    VkImageView getTextureImageView() const;

private:
    Device* device_;
    VmaAllocator allocator_;
    CommandPool* commandPool_;
    std::string texturePath_;
    VkPhysicalDevice physicalDevice_;

    Image* textureImage_;
    VkImageView textureImageView_;

    static VkSampler textureSampler_;
    static size_t samplerUsers_; // Reference count for the sampler
};
