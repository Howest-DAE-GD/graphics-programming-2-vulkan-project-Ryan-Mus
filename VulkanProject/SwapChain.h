#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class SwapChain 
{
public:
    SwapChain(VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR swapChain,
              std::vector<VkImage> images, std::vector<VkImageView> imageViews,
              VkFormat imageFormat, VkExtent2D extent);
    ~SwapChain();

    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    SwapChain(SwapChain&& other) noexcept;
    SwapChain& operator=(SwapChain&& other) noexcept;

    VkSwapchainKHR get() const;
    const std::vector<VkImage>& getImages() const;
    const std::vector<VkImageView>& getImageViews() const;
    VkFormat getImageFormat() const;
    VkExtent2D getExtent() const;

private:
    VkDevice m_Device;
    VkSurfaceKHR m_Surface;
    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    VkFormat m_ImageFormat;
    VkExtent2D m_Extent;
};
