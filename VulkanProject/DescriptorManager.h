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

    void createFinalPassDescriptorSetLayout();

    void createFinalPassDescriptorSet(
        size_t frameIndex,
        VkImageView diffuseImageView,
        VkImageView normalImageView,
        VkImageView metallicRoughnessImageView,
        VkImageView depthImageView,
        VkBuffer uniformBuffer,
        size_t uniformBufferObjectSize,
		VkBuffer lightBuffer,
		size_t lightBufferObjectSize,
        VkSampler sampler
    );

    void updateFinalPassDescriptorSet(
        size_t frameIndex,
        VkImageView diffuseImageView,
        VkImageView normalImageView,
        VkImageView metallicRoughnessImageView,
        VkImageView depthImageView,
        VkBuffer uniformBuffer,
        size_t uniformBufferObjectSize,
        VkBuffer lightBuffer,
        size_t lightBufferObjectSize,
        VkSampler sampler
    );

    VkDescriptorSetLayout getDescriptorSetLayout() const;
    VkDescriptorSetLayout getFinalPassDescriptorSetLayout() const;
    const std::vector<VkDescriptorSet>& getDescriptorSets() const;
    const std::vector<VkDescriptorSet>& getFinalPassDescriptorSets() const;

private:
    VkDevice m_Device;
    size_t m_MaxFramesInFlight;
    size_t m_MaterialCount;

    VkDescriptorSetLayout m_DescriptorSetLayout{};
    VkDescriptorSetLayout m_FinalPassDescriptorSetLayout{}; // New member
    VkDescriptorPool m_DescriptorPool{};
    std::vector<VkDescriptorSet> m_DescriptorSets{};
    std::vector<VkDescriptorSet> m_FinalPassDescriptorSets; // New member
};
