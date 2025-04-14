#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

class DescriptorSet {
public:
    DescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
                  const std::vector<VkBuffer>& uniformBuffers, VkImageView textureImageView, VkSampler textureSampler);
    ~DescriptorSet();

    const std::vector<VkDescriptorSet>& getDescriptorSets() const;

private:
    VkDevice device_;
    std::vector<VkDescriptorSet> descriptorSets_;
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
