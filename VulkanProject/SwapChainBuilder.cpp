#include "SwapChainBuilder.h"
#include "SwapChain.h"
#include <algorithm>
#include <stdexcept>
#include <spdlog/spdlog.h>

SwapChainBuilder& SwapChainBuilder::setDevice(VkDevice device) {
    device_ = device;
    return *this;
}

SwapChainBuilder& SwapChainBuilder::setPhysicalDevice(VkPhysicalDevice physicalDevice) {
    physicalDevice_ = physicalDevice;
    return *this;
}

SwapChainBuilder& SwapChainBuilder::setSurface(VkSurfaceKHR surface) {
    surface_ = surface;
    return *this;
}

SwapChainBuilder& SwapChainBuilder::setWidth(uint32_t width) {
    width_ = width;
    return *this;
}

SwapChainBuilder& SwapChainBuilder::setHeight(uint32_t height) {
    height_ = height;
    return *this;
}

SwapChainBuilder& SwapChainBuilder::setGraphicsFamilyIndex(uint32_t index) {
    graphicsFamilyIndex_ = index;
    return *this;
}

SwapChainBuilder& SwapChainBuilder::setPresentFamilyIndex(uint32_t index) {
    presentFamilyIndex_ = index;
    return *this;
}

SwapChain SwapChainBuilder::build() {
    if (device_ == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE || surface_ == VK_NULL_HANDLE) {
        throw std::runtime_error("SwapChainBuilder: Missing required parameters.");
    }

    auto swapChainSupport = querySwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent                = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface_;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { graphicsFamilyIndex_, presentFamilyIndex_ };

    if (graphicsFamilyIndex_ != presentFamilyIndex_) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;
    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("SwapChainBuilder: Failed to create swap chain.");
    }

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(device_, swapChain, &swapChainImageCount, nullptr);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(device_, swapChain, &swapChainImageCount, images.data());

    std::vector<VkImageView> imageViews{};
    imageViews.resize(images.size());

    for (size_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = images[i];
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = surfaceFormat.format;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device_, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("SwapChainBuilder: Failed to create image views.");
        }
         // Log the creation of each image view
        spdlog::info("Created image view for swap chain image {}: {}", i, static_cast<void*>(imageViews[i]));


        // Validate that the image view is not null
        if (imageViews[i] == VK_NULL_HANDLE) {
            throw std::runtime_error("SwapChainBuilder: Created image view is null for swap chain image " + std::to_string(i));
        }

        if (graphicsFamilyIndex_ != presentFamilyIndex_) {
            spdlog::info("Using different queue families for graphics ({}) and presentation ({})",
                graphicsFamilyIndex_, presentFamilyIndex_);
            // Make sure ownership transfers are handled properly
        }

    }

    return SwapChain(device_, surface_, swapChain, images, imageViews,
        surfaceFormat.format, extent);
}

SwapChainBuilder::SwapChainSupportDetails SwapChainBuilder::querySwapChainSupport() {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR SwapChainBuilder::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format     == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR SwapChainBuilder::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChainBuilder::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { width_, height_ };

        actualExtent.width  = std::max(capabilities.minImageExtent.width,
                                       std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}
