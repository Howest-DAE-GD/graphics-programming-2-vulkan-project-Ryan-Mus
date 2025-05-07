// Image.cpp

#include "Image.h"
#include "CommandPool.h"
#include "Device.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

Image::Image(Device* device, VmaAllocator allocator)
    : m_pDevice(device), m_Allocator(allocator), m_Image(VK_NULL_HANDLE), m_Allocation(VK_NULL_HANDLE) 
{
	spdlog::debug("Image created.");
}

Image::~Image() {
    if (m_Image != VK_NULL_HANDLE) 
    {
        vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
    }
	spdlog::debug("Image destroyed.");
}

void Image::createImage(uint32_t width, uint32_t height,
                        VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VmaMemoryUsage memoryUsage) 
{
	createImage(width, height, format, tiling, usage, 0, 1,memoryUsage);
}

void Image::createImage(
    uint32_t width, uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
	size_t layerCount,
    VmaMemoryUsage memoryUsage
    )
{
	m_Width = width;
	m_Height = height;
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = { width, height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = layerCount;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = flags;
	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = memoryUsage;
	if (vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &m_Image, &m_Allocation, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}
	spdlog::debug("Image created with width: {}, height: {}", width, height);
}

VkImageView Image::createImageView(VkFormat format, VkImageAspectFlags aspectFlags) 
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_pDevice->get(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create texture image view!");
    }

	spdlog::debug("Image view created.");

    return imageView;
}

void Image::transitionImageLayout(CommandPool* commandPool, VkQueue graphicsQueue,
    VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = commandPool->beginSingleTimeCommands();

    VkPipelineStageFlags2 srcStageMask = 0;
    VkPipelineStageFlags2 dstStageMask = 0;
    VkAccessFlags2 srcAccessMask = 0;
    VkAccessFlags2 dstAccessMask = 0;

    // Handle different layout transitions
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        srcAccessMask = 0;
        dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {

		srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		srcAccessMask = 0;
		dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	}
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_Image;

    // Correctly set the aspect mask based on the format
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (hasStencilComponent(format)) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    commandPool->endSingleTimeCommands(commandBuffer, graphicsQueue);

	setImageLayout(newLayout);
}


void Image::copyBufferToImage(CommandPool* commandPool, VkBuffer buffer, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = commandPool->beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    commandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
	spdlog::debug("Buffer copied to image.");
}

VkImage Image::getImage() const 
{
    return m_Image;
}

VmaAllocation Image::getAllocation() const 
{
    return m_Allocation;
}
