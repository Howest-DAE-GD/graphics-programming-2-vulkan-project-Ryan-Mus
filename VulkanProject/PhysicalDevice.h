#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

class PhysicalDevice {
public:
    PhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                   const std::vector<const char*>& requiredExtensions,
                   const VkPhysicalDeviceFeatures& requiredFeatures);
    ~PhysicalDevice() = default;

    // Delete copy constructor and copy assignment operator
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;

    // Move constructor
    PhysicalDevice(PhysicalDevice&& other) noexcept;

    // Move assignment operator
    PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;

    VkPhysicalDevice get() const;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const;
    };

    const QueueFamilyIndices& getQueueFamilyIndices() const;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport() const;

    const VkPhysicalDeviceFeatures& getFeatures() const;
    const std::vector<const char*>& getExtensions() const;


private:
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physicalDevice_{ VK_NULL_HANDLE };
    QueueFamilyIndices queueFamilyIndices_;
    SwapChainSupportDetails swapChainSupportDetails_;

    std::vector<const char*> requiredExtensions_;
    VkPhysicalDeviceFeatures requiredFeatures_;
};
