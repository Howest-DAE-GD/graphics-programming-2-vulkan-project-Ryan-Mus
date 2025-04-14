#include "SwapChain.h"
#include <spdlog/spdlog.h>

SwapChain::SwapChain(VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR swapChain,
                     std::vector<VkImage> images, std::vector<VkImageView> imageViews,
                     VkFormat imageFormat, VkExtent2D extent)
    : device_(device), surface_(surface), swapChain_(swapChain),
    images_(images), imageViews_(imageViews),
    imageFormat_(imageFormat), extent_(extent) {
}

SwapChain::~SwapChain() 
{
    spdlog::info("SwapChain destructor called, destroying {} image views", imageViews_.size());

    for (auto imageView : imageViews_) {
        spdlog::info("Destroying image view: {}", static_cast<void*>(imageView));
        vkDestroyImageView(device_, imageView, nullptr);
    }

    if (swapChain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
    }
}

// Move constructor implementation
SwapChain::SwapChain(SwapChain&& other) noexcept
    : device_(other.device_), surface_(other.surface_), swapChain_(other.swapChain_),
    images_(std::move(other.images_)), imageViews_(std::move(other.imageViews_)),
    imageFormat_(other.imageFormat_), extent_(other.extent_) {
    other.swapChain_ = VK_NULL_HANDLE;
}

// Move assignment operator implementation
SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
    if (this != &other) {
        device_ = other.device_;
        surface_ = other.surface_;
        swapChain_ = other.swapChain_;
        images_ = std::move(other.images_);
        imageViews_ = std::move(other.imageViews_);
        imageFormat_ = other.imageFormat_;
        extent_ = other.extent_;

        other.swapChain_ = VK_NULL_HANDLE;
    }
    return *this;
}

VkSwapchainKHR SwapChain::get() const {
    return swapChain_;
}

const std::vector<VkImage>& SwapChain::getImages() const {
    return images_;
}

const std::vector<VkImageView>& SwapChain::getImageViews() const {
    return imageViews_;
}

VkFormat SwapChain::getImageFormat() const {
    return imageFormat_;
}

VkExtent2D SwapChain::getExtent() const {
    return extent_;
}
