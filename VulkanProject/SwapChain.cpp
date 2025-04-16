#include "SwapChain.h"
#include <spdlog/spdlog.h>

SwapChain::SwapChain(VkDevice device, VkSurfaceKHR surface, VkSwapchainKHR swapChain,
                     std::vector<VkImage> images, std::vector<VkImageView> imageViews,
                     VkFormat imageFormat, VkExtent2D extent)
    : m_Device(device), m_Surface(surface), m_SwapChain(swapChain),
    m_Images(images), m_ImageViews(imageViews),
    m_ImageFormat(imageFormat), m_Extent(extent) 
{
	spdlog::debug("SwapChain created with {} images and {} image views", m_Images.size(), m_ImageViews.size());
	spdlog::debug("SwapChain extent: {}x{}", m_Extent.width, m_Extent.height);
}

SwapChain::~SwapChain() 
{
    spdlog::debug("SwapChain destructor called, destroying {} image views", m_ImageViews.size());

    for (auto imageView : m_ImageViews) 
    {
        spdlog::debug("Destroying image view: {}", static_cast<void*>(imageView));
        vkDestroyImageView(m_Device, imageView, nullptr);
    }

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    }
	spdlog::debug("SwapChain destroyed");
}


SwapChain::SwapChain(SwapChain&& other) noexcept
    : m_Device(other.m_Device), m_Surface(other.m_Surface), m_SwapChain(other.m_SwapChain),
    m_Images(std::move(other.m_Images)), m_ImageViews(std::move(other.m_ImageViews)),
    m_ImageFormat(other.m_ImageFormat), m_Extent(other.m_Extent) 
{
    other.m_SwapChain = VK_NULL_HANDLE;
}

SwapChain& SwapChain::operator=(SwapChain&& other) noexcept {
    if (this != &other) 
    {
        m_Device = other.m_Device;
        m_Surface = other.m_Surface;
        m_SwapChain = other.m_SwapChain;
        m_Images = std::move(other.m_Images);
        m_ImageViews = std::move(other.m_ImageViews);
        m_ImageFormat = other.m_ImageFormat;
        m_Extent = other.m_Extent;

        other.m_SwapChain = VK_NULL_HANDLE;
    }
    return *this;
}

VkSwapchainKHR SwapChain::get() const 
{
    return m_SwapChain;
}

const std::vector<VkImage>& SwapChain::getImages() const 
{
    return m_Images;
}

const std::vector<VkImageView>& SwapChain::getImageViews() const 
{
    return m_ImageViews;
}

VkFormat SwapChain::getImageFormat() const 
{
    return m_ImageFormat;
}

VkExtent2D SwapChain::getExtent() const 
{
    return m_Extent;
}
