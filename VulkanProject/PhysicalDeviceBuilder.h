#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class PhysicalDevice;

class PhysicalDeviceBuilder {
public:
    PhysicalDeviceBuilder& setInstance(VkInstance instance);
    PhysicalDeviceBuilder& setSurface(VkSurfaceKHR surface);
    PhysicalDeviceBuilder& addRequiredExtension(const char* extension);
    PhysicalDeviceBuilder& setRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& features);
    PhysicalDevice build();

private:
    VkInstance instance_{ VK_NULL_HANDLE };
    VkSurfaceKHR surface_{ VK_NULL_HANDLE };
    std::vector<const char*> requiredExtensions_;
    VkPhysicalDeviceFeatures requiredFeatures_{};
};
