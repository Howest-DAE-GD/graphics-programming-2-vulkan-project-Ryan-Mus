#pragma once

#include <vulkan/vulkan.h>

class DescriptorPool {
public:
    DescriptorPool(VkDevice device, uint32_t maxFrames);
    ~DescriptorPool();

    VkDescriptorPool get() const;

private:
    VkDevice device_;
    VkDescriptorPool descriptorPool_;
};
