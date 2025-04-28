// DescriptorManager.h
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "Material.h"

class DescriptorManager
{
public:
    DescriptorManager(VkDevice device, size_t maxFramesInFlight, size_t materialCount);
    ~DescriptorManager();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets(
        const std::vector<VkBuffer>& uniformBuffers,
        const std::vector<Material*>& materials,
        size_t uniformBufferObjectSize
    );

    // New methods for the final pass
    void createFinalPassDescriptorSetLayout();
    void createFinalPassDescriptorSet(VkImageView diffuseImageView, VkImageView specularImageView, VkSampler sampler);

    VkDescriptorSetLayout getDescriptorSetLayout() const;
    VkDescriptorSetLayout getFinalPassDescriptorSetLayout() const; // New getter
    const std::vector<VkDescriptorSet>& getDescriptorSets() const;
    const VkDescriptorSet& getFinalPassDescriptorSet() const; // New getter

private:
    VkDevice m_Device;
    size_t m_MaxFramesInFlight;
    size_t m_MaterialCount;

    VkDescriptorSetLayout m_DescriptorSetLayout{};
    VkDescriptorSetLayout m_FinalPassDescriptorSetLayout{}; // New member
    VkDescriptorPool m_DescriptorPool{};
    std::vector<VkDescriptorSet> m_DescriptorSets{};
    VkDescriptorSet m_FinalPassDescriptorSet{}; // New member
};
