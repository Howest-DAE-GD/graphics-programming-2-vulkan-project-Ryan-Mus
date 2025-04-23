#include "Instance.h"
#include "InstanceBuilder.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <iostream>
#include <stdexcept>
#include <cstring>


Instance::Instance() 
{
    if (m_EnableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    // Application Info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Required Extensions
    auto extensions = getRequiredExtensions();

    // Debug Messenger Create Info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_EnableValidationLayers) 
    {
        populateDebugMessengerCreateInfo(debugCreateInfo);
    }

    // InstanceBuilder
    InstanceBuilder instanceBuilder;
    instanceBuilder.setApplicationInfo(appInfo)
        .enableExtensions(extensions);

    if (m_EnableValidationLayers)
    {
        instanceBuilder.enableValidationLayers(m_ValidationLayers)
            .setDebugMessengerCreateInfo(debugCreateInfo);
    }

    // Build the Vulkan instance
    if (instanceBuilder.build(m_Instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    // Set up the debug messenger if validation layers are enabled
    setupDebugMessenger();

    // Log used extensions
    spdlog::info("Used extensions:");
    for (const auto& extension : extensions) 
    {
        spdlog::info("\t{}", extension);
    }

	spdlog::debug("Instance created.");
}

Instance::~Instance() 
{
    if (m_EnableValidationLayers) 
    {
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }
    vkDestroyInstance(m_Instance, nullptr);
	spdlog::debug("Destroying Instance.");
}

VkInstance Instance::getInstance() const 
{
    return m_Instance;
}

void Instance::setupDebugMessenger() 
{
    if (!m_EnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
	spdlog::debug("Debug messenger created.");
}

void Instance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
}

bool Instance::checkValidationLayerSupport() 
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_ValidationLayers) 
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0) 
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) 
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> Instance::getRequiredExtensions() 
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_EnableValidationLayers) 
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

// Debug callback implementation
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT              messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
    void*                                        pUserData) 
{
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// Load Vulkan extension functions
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance                                  instance,
    const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkDebugUtilsMessengerEXT*                   pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) 
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else 
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance                                  instance,
    VkDebugUtilsMessengerEXT                    debugMessenger,
    const VkAllocationCallbacks*                pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) 
    {
        func(instance, debugMessenger, pAllocator);
    }
}
