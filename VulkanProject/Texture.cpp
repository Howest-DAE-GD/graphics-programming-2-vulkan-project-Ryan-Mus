#include "Texture.h"
#include "Buffer.h"
#include "Device.h"
#include <stb_image.h>
#include <stdexcept>

VkSampler Texture::textureSampler_ = VK_NULL_HANDLE;
size_t Texture::samplerUsers_ = 0;

Texture::Texture(Device* device, VmaAllocator allocator, CommandPool* commandPool,
                 const std::string& texturePath, VkPhysicalDevice physicalDevice)
    : device_(device), allocator_(allocator), commandPool_(commandPool),
      texturePath_(texturePath), physicalDevice_(physicalDevice),
      textureImage_(nullptr), textureImageView_(VK_NULL_HANDLE) {

    createTextureImage();
    createTextureImageView();

    // Increase sampler user count
    if (textureSampler_ == VK_NULL_HANDLE) {
        createTextureSampler(device_->get(), physicalDevice_);
    }
    samplerUsers_++;
}

Texture::~Texture() {
    vkDestroyImageView(device_->get(), textureImageView_, nullptr);
    delete textureImage_;
    samplerUsers_--;

    // Destroy sampler if no more users
    if (samplerUsers_ == 0 && textureSampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_->get(), textureSampler_, nullptr);
        textureSampler_ = VK_NULL_HANDLE;
    }
}

void Texture::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    // Create staging buffer
    Buffer stagingBuffer(
        allocator_,
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
    textureImage_ = new Image(device_, allocator_);
    textureImage_->createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Transition image layouts and copy buffer to image
    textureImage_->transitionImageLayout(
        commandPool_->get(),
        device_->getGraphicsQueue(),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    textureImage_->copyBufferToImage(
        commandPool_,
        stagingBuffer.get(),
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    textureImage_->transitionImageLayout(
        commandPool_->get(),
        device_->getGraphicsQueue(),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void Texture::createTextureImageView() {
    textureImageView_ = textureImage_->createImageView(
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
}

void Texture::createTextureSampler(VkDevice device, VkPhysicalDevice physicalDevice) {
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

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

VkSampler Texture::getTextureSampler() {
    return textureSampler_;
}

VkImageView Texture::getTextureImageView() const {
    return textureImageView_;
}
