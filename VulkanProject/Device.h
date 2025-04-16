#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <set>

class PhysicalDevice;

class Device 
{
public:
    Device(VkDevice device, VkQueue graphicsQueue, VkQueue presentQueue);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    VkDevice get() const;
    VkQueue getGraphicsQueue() const;
    VkQueue getPresentQueue() const;
private:
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
};
