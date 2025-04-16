#include "Device.h"
#include "PhysicalDevice.h"
#include <spdlog/spdlog.h>

Device::Device(VkDevice device, VkQueue graphicsQueue, VkQueue presentQueue)
    : m_Device(device), m_GraphicsQueue(graphicsQueue), m_PresentQueue(presentQueue) 
{
	spdlog::debug("Device created successfully.");
}

Device::~Device() 
{
    if (m_Device != VK_NULL_HANDLE) 
    {
        vkDestroyDevice(m_Device, nullptr);
    }
	spdlog::debug("Device destroyed.");
}

VkDevice Device::get() const 
{
    return m_Device;
}

VkQueue Device::getGraphicsQueue() const 
{
    return m_GraphicsQueue;
}

VkQueue Device::getPresentQueue() const 
{
    return m_PresentQueue;
}

Device::Device(Device&& other) noexcept 
{
    m_Device = other.m_Device;
    m_GraphicsQueue = other.m_GraphicsQueue;
    m_PresentQueue = other.m_PresentQueue;

    other.m_Device = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept 
{
    if (this != &other) 
    {
        if (m_Device != VK_NULL_HANDLE) 
        {
            vkDestroyDevice(m_Device, nullptr);
        }
        m_Device = other.m_Device;
        m_GraphicsQueue = other.m_GraphicsQueue;
        m_PresentQueue = other.m_PresentQueue;

        other.m_Device = VK_NULL_HANDLE;
    }
    return *this;
}