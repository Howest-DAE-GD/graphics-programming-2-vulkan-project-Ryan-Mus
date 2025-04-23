#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

class PhysicalDevice
{
public:
    PhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
        const std::vector<const char*>& requiredExtensions,
        const VkPhysicalDeviceFeatures& requiredFeatures,
        const VkPhysicalDeviceVulkan11Features* vulkan11Features = nullptr,
        const VkPhysicalDeviceVulkan12Features* vulkan12Features = nullptr,
        const VkPhysicalDeviceVulkan13Features* vulkan13Features = nullptr);
    ~PhysicalDevice() = default;

    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&& other) noexcept;
    PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;

    VkPhysicalDevice get() const;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const;
    };

    const QueueFamilyIndices& getQueueFamilyIndices() const;

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport() const;

    const VkPhysicalDeviceFeatures& getFeatures() const;
    const VkPhysicalDeviceVulkan11Features& getVulkan11Features() const;
    const VkPhysicalDeviceVulkan12Features& getVulkan12Features() const;
    const VkPhysicalDeviceVulkan13Features& getVulkan13Features() const;
    const std::vector<const char*>& getExtensions() const;

private:
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkFeatureSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    VkInstance m_Instance;
    VkSurfaceKHR m_Surface;
    VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
    QueueFamilyIndices m_QueueFamilyIndices;
    SwapChainSupportDetails m_SwapChainSupportDetails;

    std::vector<const char*> m_RequiredExtensions;
    VkPhysicalDeviceFeatures m_RequiredFeatures{};
    VkPhysicalDeviceVulkan11Features m_Vulkan11Features{};
    VkPhysicalDeviceVulkan12Features m_Vulkan12Features{};
    VkPhysicalDeviceVulkan13Features m_Vulkan13Features{};

    bool m_UseVulkan11Features{ false };
    bool m_UseVulkan12Features{ false };
    bool m_UseVulkan13Features{ false };
};
