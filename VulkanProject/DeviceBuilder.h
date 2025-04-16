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
    DeviceBuilder& enableValidationLayers(const std::vector<const char*>& validationLayers);

    Device* build();

private:
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    PhysicalDevice::QueueFamilyIndices m_QueueFamilyIndices;
    std::vector<const char*> m_RequiredExtensions;
    VkPhysicalDeviceFeatures m_EnabledFeatures_{};
    std::vector<const char*> m_ValidationLayers;
};