#include "PhysicalDevice.h"
#include <stdexcept>
#include <set>
#include <spdlog/spdlog.h>

PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
    const std::vector<const char*>& requiredExtensions,
    const VkPhysicalDeviceFeatures& requiredFeatures,
    const VkPhysicalDeviceVulkan11Features* vulkan11Features,
    const VkPhysicalDeviceVulkan12Features* vulkan12Features,
    const VkPhysicalDeviceVulkan13Features* vulkan13Features)
    : m_Instance(instance), m_Surface(surface),
    m_RequiredExtensions(requiredExtensions),
    m_RequiredFeatures(requiredFeatures)
{
    if (vulkan11Features)
    {
        m_Vulkan11Features = *vulkan11Features;
        m_UseVulkan11Features = true;
    }

    if (vulkan12Features)
    {
        m_Vulkan12Features = *vulkan12Features;
        m_UseVulkan12Features = true;
    }

    if (vulkan13Features)
    {
        m_Vulkan13Features = *vulkan13Features;
        m_UseVulkan13Features = true;
    }

    pickPhysicalDevice();
	spdlog::debug("PhysicalDevice created.");
}

// Move constructor
PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept
    : m_Instance(other.m_Instance),
    m_Surface(other.m_Surface),
    m_PhysicalDevice(other.m_PhysicalDevice),
    m_QueueFamilyIndices(std::move(other.m_QueueFamilyIndices)),
    m_SwapChainSupportDetails(std::move(other.m_SwapChainSupportDetails)),
    m_RequiredExtensions(std::move(other.m_RequiredExtensions)),
	m_RequiredFeatures(other.m_RequiredFeatures),
	m_Vulkan11Features(other.m_Vulkan11Features),
	m_Vulkan12Features(other.m_Vulkan12Features),
	m_Vulkan13Features(other.m_Vulkan13Features),
	m_UseVulkan11Features(other.m_UseVulkan11Features),
	m_UseVulkan12Features(other.m_UseVulkan12Features),
	m_UseVulkan13Features(other.m_UseVulkan13Features)
{
    other.m_PhysicalDevice = VK_NULL_HANDLE;
}

// Move assignment operator
PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept
{
    if (this != &other)
    {
        m_Instance = other.m_Instance;
        m_Surface = other.m_Surface;
        m_PhysicalDevice = other.m_PhysicalDevice;
        m_QueueFamilyIndices = std::move(other.m_QueueFamilyIndices);
        m_SwapChainSupportDetails = std::move(other.m_SwapChainSupportDetails);
        m_RequiredExtensions = std::move(other.m_RequiredExtensions);
        m_RequiredFeatures = other.m_RequiredFeatures;
		m_Vulkan11Features = other.m_Vulkan11Features;
		m_Vulkan12Features = other.m_Vulkan12Features;
		m_Vulkan13Features = other.m_Vulkan13Features;
		m_UseVulkan11Features = other.m_UseVulkan11Features;
		m_UseVulkan12Features = other.m_UseVulkan12Features;
		m_UseVulkan13Features = other.m_UseVulkan13Features;

        other.m_PhysicalDevice = VK_NULL_HANDLE;
    }
    return *this;
}

VkPhysicalDevice PhysicalDevice::get() const
{
    return m_PhysicalDevice;
}

void PhysicalDevice::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
	spdlog::info("Found {} devices", deviceCount);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        spdlog::info("Checking device: {}", deviceProperties.deviceName);

        m_PhysicalDevice = device;
        if (isDeviceSuitable(device))
        {
            spdlog::info("Found suitable device: {}", deviceProperties.deviceName);
            m_QueueFamilyIndices = findQueueFamilies(device);
            m_SwapChainSupportDetails = querySwapChainSupport();
            break;
        }
    }
    if (!checkFeatureSupport(m_PhysicalDevice))
    {
        throw std::runtime_error("Required features are not supported by the selected physical device.");
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

bool PhysicalDevice::checkFeatureSupport(VkPhysicalDevice device)
{
    VkPhysicalDeviceFeatures2 supportedFeatures2{};
    supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceVulkan11Features supportedVulkan11Features{};
    supportedVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    VkPhysicalDeviceVulkan12Features supportedVulkan12Features{};
    supportedVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceVulkan13Features supportedVulkan13Features{};
    supportedVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    // Chain the features
	supportedFeatures2.pNext = &supportedVulkan11Features;
	supportedVulkan11Features.pNext = &supportedVulkan12Features;
	supportedVulkan12Features.pNext = &supportedVulkan13Features;

    vkGetPhysicalDeviceFeatures2(device, &supportedFeatures2);

    // Check Vulkan 1.0 features
    if (m_RequiredFeatures.samplerAnisotropy && !supportedFeatures2.features.samplerAnisotropy)
    {
        return false;
    }
 
    // Check Vulkan 1.1 features
    if (m_UseVulkan11Features)
    {
        // ... check other Vulkan 1.1 features
    }

    // Check Vulkan 1.2 features
    if (m_UseVulkan12Features)
    {
        if (m_Vulkan12Features.runtimeDescriptorArray && !supportedVulkan12Features.runtimeDescriptorArray)
        {
            return false;
        }
        // ... check other Vulkan 1.2 features
    }

    // Check Vulkan 1.3 features
    if (m_UseVulkan13Features)
    {
		if (m_Vulkan13Features.synchronization2 && !supportedVulkan13Features.synchronization2)
		{
			return false;
		}
        // ... check Vulkan 1.3 features
    }

    return true;
}

bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device)
{
    auto indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        auto swapChainSupport = querySwapChainSupport();
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    return indices.isComplete()
        && extensionsSupported
        && swapChainAdequate
        && supportedFeatures.samplerAnisotropy == m_RequiredFeatures.samplerAnisotropy
        && deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

PhysicalDevice::QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }
        i++;
    }

    return indices;
}

bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_RequiredExtensions.begin(), m_RequiredExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

PhysicalDevice::SwapChainSupportDetails PhysicalDevice::querySwapChainSupport() const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool PhysicalDevice::QueueFamilyIndices::isComplete() const
{
    return graphicsFamily.has_value() && presentFamily.has_value();
}

const PhysicalDevice::QueueFamilyIndices& PhysicalDevice::getQueueFamilyIndices() const
{
    return m_QueueFamilyIndices;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::getFeatures() const
{
    return m_RequiredFeatures;
}

const std::vector<const char*>& PhysicalDevice::getExtensions() const
{
    return m_RequiredExtensions;
}

const VkPhysicalDeviceVulkan11Features& PhysicalDevice::getVulkan11Features() const
{
	return m_Vulkan11Features;
}

const VkPhysicalDeviceVulkan12Features& PhysicalDevice::getVulkan12Features() const
{
	return m_Vulkan12Features;
}

const VkPhysicalDeviceVulkan13Features& PhysicalDevice::getVulkan13Features() const
{
	return m_Vulkan13Features;
}

