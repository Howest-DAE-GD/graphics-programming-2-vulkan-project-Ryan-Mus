#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Texture;
class DescriptorManager 
{
public:
    DescriptorManager(VkDevice device, size_t maxFramesInFlight);
    ~DescriptorManager();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets(
        const std::vector<VkBuffer>& uniformBuffers,
        const std::vector<Texture*>& textures,
        size_t uniformBufferObjectSize
    );

    VkDescriptorSetLayout getDescriptorSetLayout() const;
    const std::vector<VkDescriptorSet>& getDescriptorSets() const;

private:
    VkDevice m_Device;
    size_t m_MaxFramesInFlight;

    VkDescriptorSetLayout m_DescriptorSetLayout{};
    VkDescriptorPool m_DescriptorPool{};
    std::vector<VkDescriptorSet> m_DescriptorSets{};
};
