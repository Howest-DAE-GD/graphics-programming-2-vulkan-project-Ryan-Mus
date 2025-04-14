#include "PhysicalDevice.h"
#include <stdexcept>
#include <set>

PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                               const std::vector<const char*>& requiredExtensions,
                               const VkPhysicalDeviceFeatures& requiredFeatures)
    : instance_(instance),
      surface_(surface),
      requiredExtensions_(requiredExtensions),
      requiredFeatures_(requiredFeatures) {
    pickPhysicalDevice();
}

// Move constructor
PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept
    : instance_(other.instance_),
    surface_(other.surface_),
    physicalDevice_(other.physicalDevice_),
    queueFamilyIndices_(std::move(other.queueFamilyIndices_)),
    swapChainSupportDetails_(std::move(other.swapChainSupportDetails_)),
    requiredExtensions_(std::move(other.requiredExtensions_)),
    requiredFeatures_(other.requiredFeatures_) {
    other.physicalDevice_ = VK_NULL_HANDLE;
}

// Move assignment operator
PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept {
    if (this != &other) {
        instance_ = other.instance_;
        surface_ = other.surface_;
        physicalDevice_ = other.physicalDevice_;
        queueFamilyIndices_ = std::move(other.queueFamilyIndices_);
        swapChainSupportDetails_ = std::move(other.swapChainSupportDetails_);
        requiredExtensions_ = std::move(other.requiredExtensions_);
        requiredFeatures_ = other.requiredFeatures_;

        other.physicalDevice_ = VK_NULL_HANDLE;
    }
    return *this;
}

VkPhysicalDevice PhysicalDevice::get() const {
    return physicalDevice_;
}

void PhysicalDevice::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& device : devices) {
        physicalDevice_ = device;
        if (isDeviceSuitable(device)) {
            queueFamilyIndices_ = findQueueFamilies(device);
            swapChainSupportDetails_ = querySwapChainSupport();
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device) {
    auto indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        auto swapChainSupport = querySwapChainSupport();
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete()
        && extensionsSupported
        && swapChainAdequate
        && supportedFeatures.samplerAnisotropy == requiredFeatures_.samplerAnisotropy;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(requiredExtensions_.begin(), requiredExtensions_.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

PhysicalDevice::SwapChainSupportDetails PhysicalDevice::querySwapChainSupport() const {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool PhysicalDevice::QueueFamilyIndices::isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
}

const PhysicalDevice::QueueFamilyIndices& PhysicalDevice::getQueueFamilyIndices() const {
    return queueFamilyIndices_;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::getFeatures() const {
    return requiredFeatures_;
}

const std::vector<const char*>& PhysicalDevice::getExtensions() const {
    return requiredExtensions_;
}
