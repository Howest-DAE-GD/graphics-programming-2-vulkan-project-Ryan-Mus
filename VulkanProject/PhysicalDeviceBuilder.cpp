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
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& features)
{
    m_RequiredFeatures = features;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setVulkan11Features(const VkPhysicalDeviceVulkan11Features& features)
{
    m_Vulkan11Features = features;
    m_UseVulkan11Features = true;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setVulkan12Features(const VkPhysicalDeviceVulkan12Features& features)
{
    m_Vulkan12Features = features;
    m_UseVulkan12Features = true;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setVulkan13Features(const VkPhysicalDeviceVulkan13Features& features)
{
    m_Vulkan13Features = features;
    m_UseVulkan13Features = true;
    return *this;
}


PhysicalDevice* PhysicalDeviceBuilder::build()
{
    PhysicalDevice* physicalDevice = new PhysicalDevice(
        m_Instance,
        m_Surface,
        m_RequiredExtensions,
        m_RequiredFeatures,
        m_UseVulkan11Features ? &m_Vulkan11Features : nullptr,
        m_UseVulkan12Features ? &m_Vulkan12Features : nullptr,
        m_UseVulkan13Features ? &m_Vulkan13Features : nullptr
    );

    return physicalDevice;
}

