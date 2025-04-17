#include "DescriptorManager.h"
#include "Texture.h"

#include <stdexcept>
#include <array>
#include <spdlog/spdlog.h>

DescriptorManager::DescriptorManager(VkDevice device, size_t maxFramesInFlight, size_t textureCount)
    : m_Device(device), m_MaxFramesInFlight(maxFramesInFlight), m_TextureCount(textureCount)
{
    this->createDescriptorSetLayout();
    this->createDescriptorPool();
    spdlog::debug("DescriptorManager created.");
}

DescriptorManager::~DescriptorManager()
{
	if (m_DescriptorSetLayout != VK_NULL_HANDLE) 
    {
		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
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

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = static_cast<uint32_t>(m_TextureCount);
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

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
	if (m_DescriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
	}
	if (m_TextureCount == 0) m_TextureCount = 1;
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_MaxFramesInFlight);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(m_MaxFramesInFlight * m_TextureCount);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_MaxFramesInFlight);

    if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
    spdlog::debug("Descriptor pool created.");
}

void DescriptorManager::createDescriptorSets(
    const std::vector<VkBuffer>& uniformBuffers,
    const std::vector<Texture*>& textures,
    size_t uniformBufferObjectSize)
{
    if (textures.size() != m_TextureCount) 
    {
        throw std::runtime_error("Texture count does not match the expected number.");
    }

    std::vector<VkDescriptorSetLayout> layouts(m_MaxFramesInFlight, m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_MaxFramesInFlight);
    allocInfo.pSetLayouts = layouts.data();

    m_DescriptorSets.resize(m_MaxFramesInFlight);
    if (vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < m_MaxFramesInFlight; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = uniformBufferObjectSize;

        std::vector<VkDescriptorImageInfo> imageInfos(m_TextureCount);
        for (size_t j = 0; j < m_TextureCount; ++j) 
        {
            imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[j].imageView = textures[j]->getTextureImageView();
            imageInfos[j].sampler = Texture::getTextureSampler();
        }

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_DescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_DescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = static_cast<uint32_t>(m_TextureCount);
        descriptorWrites[1].pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(
            m_Device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0,
            nullptr
        );
        spdlog::debug("Descriptor set updated for frame {}", i);
    }
}

void DescriptorManager::SetTextureCount(size_t textureCount)
{
	m_TextureCount = textureCount;
	createDescriptorSetLayout();
	createDescriptorPool();
}

VkDescriptorSetLayout DescriptorManager::getDescriptorSetLayout() const
{
    return m_DescriptorSetLayout;
}

const std::vector<VkDescriptorSet>& DescriptorManager::getDescriptorSets() const
{
	return m_DescriptorSets;
}

