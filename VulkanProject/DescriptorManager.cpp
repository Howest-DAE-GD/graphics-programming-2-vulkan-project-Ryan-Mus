// DescriptorManager.cpp
#include "DescriptorManager.h"
#include <array>
#include <stdexcept>
#include <spdlog/spdlog.h>

DescriptorManager::DescriptorManager(VkDevice device, size_t maxFramesInFlight, size_t materialCount)
    : m_Device(device), m_MaxFramesInFlight(maxFramesInFlight), m_MaterialCount(materialCount)
{
    //createDescriptorSetLayout();
    //createDescriptorPool();
    spdlog::debug("DescriptorManager created.");
}

DescriptorManager::~DescriptorManager()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    }
 
	if (m_FinalPassDescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_Device, m_FinalPassDescriptorSetLayout, nullptr);
	}

    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    }
    spdlog::debug("DescriptorManager destroyed.");
}

void DescriptorManager::createDescriptorSetLayout()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    }

    // Binding for Uniform Buffer Object
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0; // Binding 0 for the UBO
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Allow usage in both shaders
    uboLayoutBinding.pImmutableSamplers = nullptr;

    // Binding for Combined Image Sampler (Diffuse Texture)
    VkDescriptorSetLayoutBinding diffuseSamplerBinding{};
    diffuseSamplerBinding.binding = 1;
    diffuseSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    diffuseSamplerBinding.descriptorCount = 1;
    diffuseSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    diffuseSamplerBinding.pImmutableSamplers = nullptr;

    // Binding for Combined Image Sampler (Specular Texture)
    VkDescriptorSetLayoutBinding specularSamplerBinding{};
    specularSamplerBinding.binding = 2;
    specularSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    specularSamplerBinding.descriptorCount = 1;
    specularSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    specularSamplerBinding.pImmutableSamplers = nullptr;

    // Binding for Combined Image Sampler (Alpha Texture)
    VkDescriptorSetLayoutBinding alphaSamplerBinding{};
    alphaSamplerBinding.binding = 3;
    alphaSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    alphaSamplerBinding.descriptorCount = 1;
    alphaSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    alphaSamplerBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
        uboLayoutBinding,
        diffuseSamplerBinding,
        specularSamplerBinding,
        alphaSamplerBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
    spdlog::debug("Descriptor set layout created.");
}

void DescriptorManager::createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        // For uniform buffers
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_MaxFramesInFlight * m_MaterialCount) },

        // For combined image samplers used in the main pass
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_MaxFramesInFlight * m_MaterialCount * 3) },

        // For combined image samplers used in the final pass
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 } // Diffuse and specular
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_MaxFramesInFlight * m_MaterialCount + 1); // +1 for the final pass

    if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}


void DescriptorManager::createDescriptorSets(
    const std::vector<VkBuffer>& uniformBuffers,
    const std::vector<Material*>& materials,
    size_t uniformBufferObjectSize)
{
    if (materials.size() != m_MaterialCount)
    {
        throw std::runtime_error("Material count does not match the expected number.");
    }

    m_DescriptorSets.resize(m_MaxFramesInFlight * m_MaterialCount);

    std::vector<VkDescriptorSetLayout> layouts(m_MaxFramesInFlight * m_MaterialCount, m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t frame = 0; frame < m_MaxFramesInFlight; frame++)
    {
        for (size_t matIndex = 0; matIndex < m_MaterialCount; ++matIndex)
        {
            size_t descriptorSetIndex = frame * m_MaterialCount + matIndex;

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[frame];
            bufferInfo.offset = 0;
            bufferInfo.range = uniformBufferObjectSize;

            // Collect image infos from the material's textures
            VkDescriptorImageInfo diffuseImageInfo{};
            diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            diffuseImageInfo.imageView = materials[matIndex]->pDiffuseTexture->getTextureImageView();
            diffuseImageInfo.sampler = Texture::getTextureSampler();

            VkDescriptorImageInfo specularImageInfo{};
            specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            specularImageInfo.imageView = materials[matIndex]->pSpecularTexture->getTextureImageView();
            specularImageInfo.sampler = Texture::getTextureSampler();

            VkDescriptorImageInfo alphaImageInfo{};
            alphaImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            alphaImageInfo.imageView = materials[matIndex]->pAlphaTexture->getTextureImageView();
            alphaImageInfo.sampler = Texture::getTextureSampler();

            std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

            // Uniform Buffer
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_DescriptorSets[descriptorSetIndex];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // Diffuse Texture
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_DescriptorSets[descriptorSetIndex];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &diffuseImageInfo;

            // Specular Texture
            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = m_DescriptorSets[descriptorSetIndex];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &specularImageInfo;

            // Alpha Texture
            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = m_DescriptorSets[descriptorSetIndex];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pImageInfo = &alphaImageInfo;

            vkUpdateDescriptorSets(
                m_Device,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr
            );
            spdlog::debug("Descriptor set updated for frame {}, material {}", frame, matIndex);
        }
    }
}


VkDescriptorSetLayout DescriptorManager::getDescriptorSetLayout() const
{
    return m_DescriptorSetLayout;
}

const std::vector<VkDescriptorSet>& DescriptorManager::getDescriptorSets() const
{
    return m_DescriptorSets;
}

void DescriptorManager::createFinalPassDescriptorSetLayout()
{
    // Binding for diffuse sampler (binding = 0)
    VkDescriptorSetLayoutBinding diffuseBinding{};
    diffuseBinding.binding = 0;
    diffuseBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    diffuseBinding.descriptorCount = 1;
    diffuseBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    diffuseBinding.pImmutableSamplers = nullptr;

    // Binding for specular sampler (binding = 1)
    VkDescriptorSetLayoutBinding specularBinding{};
    specularBinding.binding = 1;
    specularBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    specularBinding.descriptorCount = 1;
    specularBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    specularBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { diffuseBinding, specularBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_FinalPassDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout for the final pass!");
    }
}

VkDescriptorSetLayout DescriptorManager::getFinalPassDescriptorSetLayout() const
{
    return m_FinalPassDescriptorSetLayout;
}

void DescriptorManager::createFinalPassDescriptorSet(VkImageView diffuseImageView, VkImageView specularImageView, VkSampler sampler)
{
    // Allocate the descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool; // Ensure the pool can allocate the required descriptors
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_FinalPassDescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_Device, &allocInfo, &m_FinalPassDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set for the final pass!");
    }

    // Update the descriptor set with the G-Buffer images
    VkDescriptorImageInfo diffuseImageInfo{};
    diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseImageInfo.imageView = diffuseImageView;
    diffuseImageInfo.sampler = sampler;

    VkDescriptorImageInfo specularImageInfo{};
    specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    specularImageInfo.imageView = specularImageView;
    specularImageInfo.sampler = sampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_FinalPassDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &diffuseImageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_FinalPassDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &specularImageInfo;

    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

const VkDescriptorSet& DescriptorManager::getFinalPassDescriptorSet() const
{
    return m_FinalPassDescriptorSet;
}

