#include "DeviceBuilder.h"
#include "Device.h"
#include <set>
#include <spdlog/spdlog.h>

DeviceBuilder& DeviceBuilder::setPhysicalDevice(VkPhysicalDevice physicalDevice) 
{
    m_PhysicalDevice = physicalDevice;
    return *this;
}

DeviceBuilder& DeviceBuilder::setQueueFamilyIndices(const PhysicalDevice::QueueFamilyIndices& indices) 
{
    m_QueueFamilyIndices = indices;
    return *this;
}

DeviceBuilder& DeviceBuilder::addRequiredExtension(const char* extension) 
{
    m_RequiredExtensions.push_back(extension);
    return *this;
}

DeviceBuilder& DeviceBuilder::setEnabledFeatures(const VkPhysicalDeviceFeatures& features) 
{
    m_EnabledFeatures = features;
    return *this;
}

DeviceBuilder& DeviceBuilder::enableValidationLayers(const std::vector<const char*>& validationLayers) 
{
    m_ValidationLayers = validationLayers;
    return *this;
}

DeviceBuilder& DeviceBuilder::setVulkan11Features(const VkPhysicalDeviceVulkan11Features& features)
{
    m_Vulkan11Features = features;
    m_UseVulkan11Features = true;
    return *this;
}

DeviceBuilder& DeviceBuilder::setVulkan12Features(const VkPhysicalDeviceVulkan12Features& features)
{
    m_Vulkan12Features = features;
    m_UseVulkan12Features = true;
    return *this;
}

DeviceBuilder& DeviceBuilder::setVulkan13Features(const VkPhysicalDeviceVulkan13Features& features)
{
    m_Vulkan13Features = features;
    m_UseVulkan13Features = true;
    return *this;
}

Device* DeviceBuilder::build()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_QueueFamilyIndices.graphicsFamily.value(),
        m_QueueFamilyIndices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = m_EnabledFeatures;

    // Chain the features
    void* pNextChain = nullptr;

    if (m_UseVulkan13Features)
    {
        m_Vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        m_Vulkan13Features.pNext = pNextChain;
        pNextChain = &m_Vulkan13Features;
    }

    if (m_UseVulkan12Features)
    {
        m_Vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        m_Vulkan12Features.pNext = pNextChain;
        pNextChain = &m_Vulkan12Features;
    }

    if (m_UseVulkan11Features)
    {
        m_Vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        m_Vulkan11Features.pNext = pNextChain;
        pNextChain = &m_Vulkan11Features;
    }

    deviceFeatures2.pNext = pNextChain;
    createInfo.pNext = &deviceFeatures2;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_RequiredExtensions.size());
    createInfo.ppEnabledExtensionNames = m_RequiredExtensions.data();

    if (!m_ValidationLayers.empty())
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkDevice device;
    if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, m_QueueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);

    VkQueue presentQueue;
    vkGetDeviceQueue(device, m_QueueFamilyIndices.presentFamily.value(), 0, &presentQueue);

    return new Device(device, graphicsQueue, presentQueue);
}

