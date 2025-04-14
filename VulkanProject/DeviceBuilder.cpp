#include "DeviceBuilder.h"
#include "Device.h"
#include <set>

DeviceBuilder& DeviceBuilder::setPhysicalDevice(VkPhysicalDevice physicalDevice) {
    physicalDevice_ = physicalDevice;
    return *this;
}

DeviceBuilder& DeviceBuilder::setQueueFamilyIndices(const PhysicalDevice::QueueFamilyIndices& indices) {
    queueFamilyIndices_ = indices;
    return *this;
}

DeviceBuilder& DeviceBuilder::addRequiredExtension(const char* extension) {
    requiredExtensions_.push_back(extension);
    return *this;
}

DeviceBuilder& DeviceBuilder::setEnabledFeatures(const VkPhysicalDeviceFeatures& features) {
    enabledFeatures_ = features;
    return *this;
}

DeviceBuilder& DeviceBuilder::enableValidationLayers(const std::vector<const char*>& validationLayers) {
    validationLayers_ = validationLayers;
    return *this;
}

Device* DeviceBuilder::build() {
    // Create the logical device
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices_.graphicsFamily.value(),
        queueFamilyIndices_.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &enabledFeatures_;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions_.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions_.data();

    if (!validationLayers_.empty()) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    VkDevice device;
    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    vkGetDeviceQueue(device, queueFamilyIndices_.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilyIndices_.presentFamily.value(), 0, &presentQueue);

    return new Device(device, graphicsQueue, presentQueue);
}