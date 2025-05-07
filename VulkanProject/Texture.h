#pragma once

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "CommandPool.h"
#include "Image.h"
#include <string>

//
// Texture sampler will probably not be static later.
//
class Device;
class Texture
{
public:
    enum class Format {
        SRGB,
        UNORM,
        HDR
    };

    Texture(Device* pDevice, VmaAllocator allocator, CommandPool* pCommandPool,
        const std::string& texturePath, VkPhysicalDevice physicalDevice, Format format = Format::SRGB);
    ~Texture();

    void createTextureImage();
    void createTextureImageView();
    VkImageView getTextureImageView() const;

    static void createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice);
    static VkSampler getTextureSampler();

private:
    Device* m_pDevice;
    VmaAllocator m_Allocator;
    CommandPool* m_pCommandPool;
    std::string m_TexturePath;
    VkPhysicalDevice m_PhysicalDevice;

    Image* m_pTextureImage;
    VkImageView m_TextureImageView;

    Format m_Format; // New member to store the texture format

    static VkSampler s_textureSampler;
    static size_t s_samplerUsers; // Reference count for the sampler
};

