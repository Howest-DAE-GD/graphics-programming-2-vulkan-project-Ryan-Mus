#pragma once
#include "PhysicalDevice.h"
#include "Surface.h"

class SwapChain;
class SwapChainBuilder 
{
public:
    SwapChainBuilder& setDevice(VkDevice device);
    SwapChainBuilder& setPhysicalDevice(VkPhysicalDevice physicalDevice);
    SwapChainBuilder& setSurface(VkSurfaceKHR surface);
    SwapChainBuilder& setWidth(uint32_t width);
    SwapChainBuilder& setHeight(uint32_t height);
    SwapChainBuilder& setGraphicsFamilyIndex(uint32_t index);
    SwapChainBuilder& setPresentFamilyIndex(uint32_t index);
	SwapChainBuilder& setImageUsage(VkImageUsageFlags usage);

    SwapChain* build();

private:
    struct SwapChainSupportDetails 
    {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkDevice m_Device{ VK_NULL_HANDLE };
    VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
    VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
    uint32_t m_Width{ 0 };
    uint32_t m_Height{ 0 };
    uint32_t m_GraphicsFamilyIndex{ 0 };
    uint32_t m_PresentFamilyIndex{ 0 };
	VkImageUsageFlags m_ImageUsage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT };
};
