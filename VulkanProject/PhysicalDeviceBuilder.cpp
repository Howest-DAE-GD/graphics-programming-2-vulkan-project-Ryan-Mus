#include "PhysicalDeviceBuilder.h"
#include "PhysicalDevice.h"
#include <stdexcept>

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setInstance(VkInstance instance) {
    instance_ = instance;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setSurface(VkSurfaceKHR surface) {
    surface_ = surface;
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::addRequiredExtension(const char* extension) {
    requiredExtensions_.push_back(extension);
    return *this;
}

PhysicalDeviceBuilder& PhysicalDeviceBuilder::setRequiredDeviceFeatures(const VkPhysicalDeviceFeatures& features) {
    requiredFeatures_ = features;
    return *this;
}

PhysicalDevice PhysicalDeviceBuilder::build() {
    if (instance_ == VK_NULL_HANDLE) {
        throw std::runtime_error("VkInstance not set in PhysicalDeviceBuilder");
    }
    if (surface_ == VK_NULL_HANDLE) {
        throw std::runtime_error("VkSurfaceKHR not set in PhysicalDeviceBuilder");
    }
    return PhysicalDevice(instance_, surface_, requiredExtensions_, requiredFeatures_);
}
