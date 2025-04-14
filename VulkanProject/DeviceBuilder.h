#pragma once
#include "PhysicalDevice.h"
#include <vector>
#include <stdexcept>

class Device;
class DeviceBuilder {
public:
    DeviceBuilder& setPhysicalDevice(VkPhysicalDevice physicalDevice);
    DeviceBuilder& setQueueFamilyIndices(const PhysicalDevice::QueueFamilyIndices& indices);
    DeviceBuilder& addRequiredExtension(const char* extension);
    DeviceBuilder& setEnabledFeatures(const VkPhysicalDeviceFeatures& features);
    DeviceBuilder& enableValidationLayers(const std::vector<const char*>& validationLayers);

    Device* build();

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    PhysicalDevice::QueueFamilyIndices queueFamilyIndices_;
    std::vector<const char*> requiredExtensions_;
    VkPhysicalDeviceFeatures enabledFeatures_{};
    std::vector<const char*> validationLayers_;
};