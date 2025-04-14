#include "Device.h"
#include "PhysicalDevice.h"

Device::Device(VkDevice device, VkQueue graphicsQueue, VkQueue presentQueue)
    : device_(device), graphicsQueue_(graphicsQueue), presentQueue_(presentQueue) {
}

Device::~Device() {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }
}

VkDevice Device::get() const {
    return device_;
}

VkQueue Device::getGraphicsQueue() const {
    return graphicsQueue_;
}

VkQueue Device::getPresentQueue() const {
    return presentQueue_;
}

Device::Device(Device&& other) noexcept {
    device_ = other.device_;
    graphicsQueue_ = other.graphicsQueue_;
    presentQueue_ = other.presentQueue_;

    other.device_ = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept {
    if (this != &other) {
        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
        }
        device_ = other.device_;
        graphicsQueue_ = other.graphicsQueue_;
        presentQueue_ = other.presentQueue_;

        other.device_ = VK_NULL_HANDLE;
    }
    return *this;
}