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

DeviceBuilder& DeviceBuilder::setVulkan12Features(const VkPhysicalDeviceVulkan12Features& features)
{
	m_Vulkan12Features = features;
    return *this;
}

DeviceBuilder& DeviceBuilder::enableValidationLayers(const std::vector<const char*>& validationLayers) 
{
    m_ValidationLayers = validationLayers;
    return *this;
}

Device* DeviceBuilder::build()
{
    // Create the logical device
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies =
    {
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

    // Set up the VkPhysicalDeviceFeatures2 structure
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = m_EnabledFeatures;

    // Set up Vulkan 1.2 features structure
    VkPhysicalDeviceVulkan12Features vulkan12Features = m_Vulkan12Features;
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.pNext = nullptr;

    // Check if VK_EXT_descriptor_indexing is enabled and set descriptorIndexing to VK_TRUE
    for (const char* extension : m_RequiredExtensions)
    {
        if (strcmp(extension, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0)
        {
            vulkan12Features.descriptorIndexing = VK_TRUE;
            break;
        }
    }

    // Link the structures
    physicalDeviceFeatures2.pNext = &vulkan12Features;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &physicalDeviceFeatures2; // Point to the features2 structure
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr; // Must be nullptr when using pNext with VkPhysicalDeviceFeatures2
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
        throw std::runtime_error("failed to create logical device!");
    }

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    vkGetDeviceQueue(device, m_QueueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, m_QueueFamilyIndices.presentFamily.value(), 0, &presentQueue);

    spdlog::debug("Logical device built with Vulkan 1.2 features enabled.");
    return new Device(device, graphicsQueue, presentQueue);
}
