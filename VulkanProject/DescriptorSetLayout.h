#pragma once

#include <vulkan/vulkan.h>

class DescriptorSetLayout {
public:
    DescriptorSetLayout(VkDevice device);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout* get() const;

private:
    VkDevice device_;
    VkDescriptorSetLayout* descriptorSetLayout_;
};
