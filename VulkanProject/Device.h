#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <set>

class PhysicalDevice;

class Device {
public:
    Device(VkDevice device, VkQueue graphicsQueue, VkQueue presentQueue);
    ~Device();

    VkDevice get() const;
    VkQueue getGraphicsQueue() const;
    VkQueue getPresentQueue() const;

    // Delete copy constructor and assignment
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Allow move semantics
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

private:
    VkDevice device_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
};
