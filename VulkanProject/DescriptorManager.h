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

    void setMaterialCount(size_t materialCount);

    VkDescriptorSetLayout getDescriptorSetLayout() const;
    const std::vector<VkDescriptorSet>& getDescriptorSets() const;

private:
    VkDevice m_Device;
    size_t m_MaxFramesInFlight;
    size_t m_MaterialCount; // Updated to store material count

    VkDescriptorSetLayout m_DescriptorSetLayout{};
    VkDescriptorPool m_DescriptorPool{};
    std::vector<VkDescriptorSet> m_DescriptorSets{};
};