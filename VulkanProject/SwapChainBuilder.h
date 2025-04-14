#pragma once
#include "PhysicalDevice.h"
#include "Surface.h"

class SwapChain;
class SwapChainBuilder {
public:
    SwapChainBuilder& setDevice(VkDevice device);
    SwapChainBuilder& setPhysicalDevice(VkPhysicalDevice physicalDevice);
    SwapChainBuilder& setSurface(VkSurfaceKHR surface);
    SwapChainBuilder& setWidth(uint32_t width);
    SwapChainBuilder& setHeight(uint32_t height);
    SwapChainBuilder& setGraphicsFamilyIndex(uint32_t index);
    SwapChainBuilder& setPresentFamilyIndex(uint32_t index);

    SwapChain build();

private:
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkDevice device_{ VK_NULL_HANDLE };
    VkPhysicalDevice physicalDevice_{ VK_NULL_HANDLE };
    VkSurfaceKHR surface_{ VK_NULL_HANDLE };
    uint32_t width_{ 0 };
    uint32_t height_{ 0 };
    uint32_t graphicsFamilyIndex_{ 0 };
    uint32_t presentFamilyIndex_{ 0 };
};
