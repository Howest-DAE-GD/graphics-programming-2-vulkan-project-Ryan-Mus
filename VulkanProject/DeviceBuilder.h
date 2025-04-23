#pragma once
#include "PhysicalDevice.h"
#include <vector>
#include <stdexcept>

class Device;

class DeviceBuilder
{
public:
    DeviceBuilder& setPhysicalDevice(VkPhysicalDevice physicalDevice);
    DeviceBuilder& setQueueFamilyIndices(const PhysicalDevice::QueueFamilyIndices& indices);
    DeviceBuilder& addRequiredExtension(const char* extension);
    DeviceBuilder& setEnabledFeatures(const VkPhysicalDeviceFeatures& features);
    DeviceBuilder& setVulkan11Features(const VkPhysicalDeviceVulkan11Features& features);
    DeviceBuilder& setVulkan12Features(const VkPhysicalDeviceVulkan12Features& features);
    DeviceBuilder& setVulkan13Features(const VkPhysicalDeviceVulkan13Features& features);
    DeviceBuilder& enableValidationLayers(const std::vector<const char*>& validationLayers);

    Device* build();

private:
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    PhysicalDevice::QueueFamilyIndices m_QueueFamilyIndices;
    std::vector<const char*> m_RequiredExtensions;
    VkPhysicalDeviceFeatures m_EnabledFeatures{};

    VkPhysicalDeviceVulkan11Features m_Vulkan11Features{};
    VkPhysicalDeviceVulkan12Features m_Vulkan12Features{};
    VkPhysicalDeviceVulkan13Features m_Vulkan13Features{};

    std::vector<const char*> m_ValidationLayers;

    bool m_UseVulkan11Features{ false };
    bool m_UseVulkan12Features{ false };
    bool m_UseVulkan13Features{ false };
};