#include "Texture.h"
#include "Buffer.h"
#include "Device.h"
#include <stb_image.h>
#include <stdexcept>
#include <spdlog/spdlog.h>

VkSampler Texture::s_textureSampler = VK_NULL_HANDLE;
size_t Texture::s_samplerUsers = 0;

Texture::Texture(Device* pDevice, VmaAllocator allocator, CommandPool* pCommandPool,
                 const std::string& texturePath, VkPhysicalDevice physicalDevice)
    : m_pDevice(pDevice), m_Allocator(allocator), m_pCommandPool(pCommandPool),
      m_TexturePath(texturePath), m_PhysicalDevice(physicalDevice),
      m_pTextureImage(nullptr), m_TextureImageView(VK_NULL_HANDLE) 
{
    createTextureImage();
    createTextureImageView();

    // Increase sampler user count
    if (s_textureSampler == VK_NULL_HANDLE) {
        createTextureSampler(m_pDevice->get(), m_PhysicalDevice);
    }
    s_samplerUsers++;

	spdlog::debug("Texture created: {}", m_TexturePath);
}

Texture::~Texture() 
{
    vkDestroyImageView(m_pDevice->get(), m_TextureImageView, nullptr);
    delete m_pTextureImage;
    s_samplerUsers--;

    // Destroy sampler if no more users
    if (s_samplerUsers == 0 && s_textureSampler != VK_NULL_HANDLE) 
    {
        vkDestroySampler(m_pDevice->get(), s_textureSampler, nullptr);
        s_textureSampler = VK_NULL_HANDLE;
    }
	spdlog::debug("Texture destroyed: {}", m_TexturePath);
}

void Texture::createTextureImage() 
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(m_TexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) 
    {
        throw std::runtime_error("Failed to load texture image!");
    }

    // Create staging buffer
    Buffer stagingBuffer(
        m_Allocator,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT
    );

    // Copy image data to staging buffer
    void* data = stagingBuffer.map();
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    // Create texture image
    m_pTextureImage = new Image(m_pDevice, m_Allocator);
    m_pTextureImage->createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Transition image layouts and copy buffer to image
    m_pTextureImage->transitionImageLayout(
        m_pCommandPool->get(),
        m_pDevice->getGraphicsQueue(),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    m_pTextureImage->copyBufferToImage(
        m_pCommandPool,
        stagingBuffer.get(),
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    m_pTextureImage->transitionImageLayout(
        m_pCommandPool->get(),
        m_pDevice->getGraphicsQueue(),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    spdlog::info("Texture image created: {} ({}x{}, {} channels)", m_TexturePath, texWidth, texHeight, texChannels);
}

void Texture::createTextureImageView() 
{
    m_TextureImageView = m_pTextureImage->createImageView(
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
}

void Texture::createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice) 
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &s_textureSampler) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
	spdlog::info("Texture sampler created");
}

VkSampler Texture::getTextureSampler() 
{
    return s_textureSampler;
}

VkImageView Texture::getTextureImageView() const 
{
    return m_TextureImageView;
}
