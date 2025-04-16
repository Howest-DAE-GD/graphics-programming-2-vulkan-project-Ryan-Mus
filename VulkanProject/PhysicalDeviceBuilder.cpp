#include "PhysicalDeviceBuilder.h"
#include "PhysicalDevice.h"
#include <stdexcept>
#include <spdlog/spdlog.h>

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setInstance(VkInstance instance) 
{
    m_Instance = instance;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setSurface(VkSurfaceKHR surface) 
{
    m_Surface = surface;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::addRequiredExtension(const char* extension) 
{
    m_RequiredExtensions.push_back(extension);
	spdlog::debug("Added required extension: {}", extension);
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& features) 
{
    m_RequiredFeatures = features;
	spdlog::debug("Set required device features.");
    return *this;
}

PhysicalDevice* PhysicalDeviceBuilder::build() 
{
    if (m_Instance == VK_NULL_HANDLE) 
    {
        throw std::runtime_error("VkInstance not set in PhysicalDeviceBuilder");
    }
    if (m_Surface == VK_NULL_HANDLE) 
    {
        throw std::runtime_error("VkSurfaceKHR not set in PhysicalDeviceBuilder");
    }
	spdlog::debug("Building PhysicalDevice.");
    return new PhysicalDevice(m_Instance, m_Surface, m_RequiredExtensions, m_RequiredFeatures);
}
