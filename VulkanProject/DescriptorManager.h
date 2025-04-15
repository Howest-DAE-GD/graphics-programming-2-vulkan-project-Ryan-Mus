#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class DescriptorManager {
public:
    DescriptorManager(VkDevice device, size_t maxFramesInFlight);
    ~DescriptorManager();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets(
        const std::vector<VkBuffer>& uniformBuffers,
        VkImageView textureImageView,
        VkSampler textureSampler,
        size_t range);

    VkDescriptorSetLayout getDescriptorSetLayout() const;
    const std::vector<VkDescriptorSet>& getDescriptorSets() const;

private:
    VkDevice device_;
    size_t maxFramesInFlight_;

    VkDescriptorSetLayout descriptorSetLayout_{};
    VkDescriptorPool descriptorPool_{};
    std::vector<VkDescriptorSet> descriptorSets_{};
};
