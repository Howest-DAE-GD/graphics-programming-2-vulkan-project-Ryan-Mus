#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class SwapChain {
public:
    SwapChain(VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR swapChain,
              std::vector<VkImage> images, std::vector<VkImageView> imageViews,
              VkFormat imageFormat, VkExtent2D extent);
    ~SwapChain();

    // Delete copy constructor and copy assignment operator
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    // Define move constructor
    SwapChain(SwapChain&& other) noexcept;
    // Define move assignment operator
    SwapChain& operator=(SwapChain&& other) noexcept;

    VkSwapchainKHR get() const;
    const std::vector<VkImage>& getImages() const;
    const std::vector<VkImageView>& getImageViews() const;
    VkFormat getImageFormat() const;
    VkExtent2D getExtent() const;

private:
    VkDevice device_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapChain_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat imageFormat_;
    VkExtent2D extent_;
};
