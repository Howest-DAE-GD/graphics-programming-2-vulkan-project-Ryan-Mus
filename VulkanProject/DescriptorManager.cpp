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
	m_FinalPassDescriptorSets.resize(maxFramesInFlight); // Initialize the final pass descriptor sets
	m_ComputeDescriptorSets.resize(maxFramesInFlight); // Initialize the compute descriptor sets
    spdlog::debug("DescriptorManager created.");
}

DescriptorManager::~DescriptorManager()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    }
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    }
    if (m_FinalPassDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_FinalPassDescriptorSetLayout, nullptr);
    }
    if (m_ComputeDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_ComputeDescriptorSetLayout, nullptr);
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

    // Binding for Combined Image Sampler (normal Texture)
    VkDescriptorSetLayoutBinding normalSamplerBinding{};
    normalSamplerBinding.binding = 2;
    normalSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalSamplerBinding.descriptorCount = 1;
    normalSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    normalSamplerBinding.pImmutableSamplers = nullptr;

    // Binding for Combined Image Sampler (metallic roughness Texture)
    VkDescriptorSetLayoutBinding metallicRoughnessSamplerBinding{};
    metallicRoughnessSamplerBinding.binding = 3;
    metallicRoughnessSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    metallicRoughnessSamplerBinding.descriptorCount = 1;
    metallicRoughnessSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    metallicRoughnessSamplerBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
        uboLayoutBinding,
        diffuseSamplerBinding,
        normalSamplerBinding,
        metallicRoughnessSamplerBinding
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
        // Total uniform buffers (main pass + final pass)
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          static_cast<uint32_t>(m_MaxFramesInFlight * (m_MaterialCount + 1)) },

          // Total combined image samplers (main pass + final pass)
          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            static_cast<uint32_t>(m_MaxFramesInFlight * (m_MaterialCount * 3 + 4)) },

            // Total storage buffers (e.g., light buffer)
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
              static_cast<uint32_t>(m_MaxFramesInFlight) },

              // Total storage images (compute descriptors)
              { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                static_cast<uint32_t>(m_MaxFramesInFlight * 2) }, // Input and output images per frame

                //// Total samplers (compute descriptors)
                //{ VK_DESCRIPTOR_TYPE_SAMPLER,
                //  static_cast<uint32_t>(m_MaxFramesInFlight) }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    // Corrected maxSets calculation
    poolInfo.maxSets = static_cast<uint32_t>(
        m_MaxFramesInFlight * (m_MaterialCount + 1) + // Main pass descriptor sets
        m_MaxFramesInFlight +                        // Final pass descriptor sets
        m_MaxFramesInFlight                          // Compute descriptor sets
        );

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

            VkDescriptorImageInfo normalImageInfo{};
            normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normalImageInfo.imageView = materials[matIndex]->pNormalTexture->getTextureImageView();
            normalImageInfo.sampler = Texture::getTextureSampler();

            VkDescriptorImageInfo metallicRoughnessImageInfo{};
            metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallicRoughnessImageInfo.imageView = materials[matIndex]->pMetallicRoughnessTexture->getTextureImageView();
            metallicRoughnessImageInfo.sampler = Texture::getTextureSampler();

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

            // Normal Texture
            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = m_DescriptorSets[descriptorSetIndex];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &normalImageInfo;

            // Metallic Roughness Texture
            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = m_DescriptorSets[descriptorSetIndex];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pImageInfo = &metallicRoughnessImageInfo;

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

    // Binding for normal sampler (binding = 1)
    VkDescriptorSetLayoutBinding normalBinding{};
    normalBinding.binding = 1;
    normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalBinding.descriptorCount = 1;
    normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    normalBinding.pImmutableSamplers = nullptr;

	// binding for metallic roughness sampler (binding = 2)
	VkDescriptorSetLayoutBinding metallicRoughnessBinding{};
	metallicRoughnessBinding.binding = 2;
	metallicRoughnessBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	metallicRoughnessBinding.descriptorCount = 1;
	metallicRoughnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	metallicRoughnessBinding.pImmutableSamplers = nullptr;

	// Binding for depth sampler (binding = 3)
	VkDescriptorSetLayoutBinding depthBinding{};
	depthBinding.binding = 3;
	depthBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	depthBinding.descriptorCount = 1;
	depthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	depthBinding.pImmutableSamplers = nullptr;

	// Binding for uniform buffer (binding = 4)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 4;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Allow usage in both shaders
    uboLayoutBinding.pImmutableSamplers = nullptr;

	// Binding for light buffer (binding = 5)
	VkDescriptorSetLayoutBinding lightBufferBinding{};
	lightBufferBinding.binding = 5;
	lightBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lightBufferBinding.descriptorCount = 1;
	lightBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	lightBufferBinding.pImmutableSamplers = nullptr;

    //Binding for skybox cubemap (binding = 6)
	VkDescriptorSetLayoutBinding skyboxBinding{};
	skyboxBinding.binding = 6;
	skyboxBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	skyboxBinding.descriptorCount = 1;
	skyboxBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	skyboxBinding.pImmutableSamplers = nullptr;

	//Binding for irradiance cubemap (binding = 7)
	VkDescriptorSetLayoutBinding irradianceBinding{};
	irradianceBinding.binding = 7;
	irradianceBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	irradianceBinding.descriptorCount = 1;
	irradianceBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	irradianceBinding.pImmutableSamplers = nullptr;

	//Binding for shadow map (binding = 8)
	VkDescriptorSetLayoutBinding shadowMapBinding{};
	shadowMapBinding.binding = 8;
	shadowMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	shadowMapBinding.descriptorCount = 1;
	shadowMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	shadowMapBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 9> bindings = { 
        diffuseBinding,
        normalBinding,
        metallicRoughnessBinding,
        depthBinding,
        uboLayoutBinding,
        lightBufferBinding,
        skyboxBinding,
		irradianceBinding,
		shadowMapBinding
    };

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

void DescriptorManager::createFinalPassDescriptorSet(
    size_t frameIndex,
    VkImageView diffuseImageView,
    VkImageView normalImageView,
    VkImageView metallicRoughnessImageView,
    VkImageView depthImageView,
    VkBuffer uniformBuffer,
    size_t uniformBufferObjectSize,
    VkBuffer lightBuffer,
    size_t lightBufferObjectSize,
	VkImageView shadowMapImageView,
	VkImageView skyboxImageView,
	VkImageView irradianceImageView,
    VkSampler sampler)
{
    // Allocate the descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool; // Ensure the pool can allocate the required descriptors
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_FinalPassDescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_Device, &allocInfo, &m_FinalPassDescriptorSets[frameIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor set for the final pass!");
    }

    // Update the descriptor set with the G-Buffer images
    VkDescriptorImageInfo diffuseImageInfo{};
    diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseImageInfo.imageView = diffuseImageView;
    diffuseImageInfo.sampler = sampler;

    VkDescriptorImageInfo normalImageInfo{};
    normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalImageInfo.imageView = normalImageView;
    normalImageInfo.sampler = sampler;

	VkDescriptorImageInfo metallicRoughnessImageInfo{};
	metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	metallicRoughnessImageInfo.imageView = metallicRoughnessImageView;
	metallicRoughnessImageInfo.sampler = sampler;

	VkDescriptorImageInfo depthImageInfo{};
	depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthImageInfo.imageView = depthImageView;
	depthImageInfo.sampler = sampler;

    VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = uniformBufferObjectSize;

	VkDescriptorBufferInfo lightBufferInfo{};
	lightBufferInfo.buffer = lightBuffer;
	lightBufferInfo.offset = 0;
	lightBufferInfo.range = lightBufferObjectSize;

	VkDescriptorImageInfo skyboxImageInfo{};
	skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyboxImageInfo.imageView = skyboxImageView;
	skyboxImageInfo.sampler = sampler;

	VkDescriptorImageInfo irradianceImageInfo{};
	irradianceImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	irradianceImageInfo.imageView = irradianceImageView;
	irradianceImageInfo.sampler = sampler;

	VkDescriptorImageInfo shadowMapImageInfo{};
	shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	shadowMapImageInfo.imageView = shadowMapImageView;
	shadowMapImageInfo.sampler = sampler;

    std::array<VkWriteDescriptorSet, 9> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_FinalPassDescriptorSets[frameIndex];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &diffuseImageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_FinalPassDescriptorSets[frameIndex];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &normalImageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &metallicRoughnessImageInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &depthImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &lightBufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &skyboxImageInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &irradianceImageInfo;

	descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[8].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[8].dstBinding = 8;
	descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[8].descriptorCount = 1;
	descriptorWrites[8].pImageInfo = &shadowMapImageInfo;

    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

const std::vector<VkDescriptorSet>& DescriptorManager::getFinalPassDescriptorSets() const
{
    return m_FinalPassDescriptorSets;
}

void DescriptorManager::updateFinalPassDescriptorSet(
    size_t frameIndex,
    VkImageView diffuseImageView,
    VkImageView normalImageView,
    VkImageView metallicRoughnessImageView,
    VkImageView depthImageView,
    VkBuffer uniformBuffer,
    size_t uniformBufferObjectSize,
    VkBuffer lightBuffer,
    size_t lightBufferObjectSize,
	VkImageView shadowMapImageView,
	VkImageView skyboxImageView,
	VkImageView irradianceImageView,
    VkSampler sampler)
{
    // Update the descriptor set with the new G-Buffer images
    VkDescriptorImageInfo diffuseImageInfo{};
    diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    diffuseImageInfo.imageView = diffuseImageView;
    diffuseImageInfo.sampler = sampler;

    VkDescriptorImageInfo normalImageInfo{};
    normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalImageInfo.imageView = normalImageView;
    normalImageInfo.sampler = sampler;

	VkDescriptorImageInfo metallicRoughnessImageInfo{};
	metallicRoughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	metallicRoughnessImageInfo.imageView = metallicRoughnessImageView;
	metallicRoughnessImageInfo.sampler = sampler;

	VkDescriptorImageInfo depthImageInfo{};
	depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	depthImageInfo.imageView = depthImageView;
	depthImageInfo.sampler = sampler;

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = uniformBufferObjectSize;

	VkDescriptorBufferInfo lightBufferInfo{};
	lightBufferInfo.buffer = lightBuffer;
	lightBufferInfo.offset = 0;
	lightBufferInfo.range = lightBufferObjectSize;

	VkDescriptorImageInfo skyboxImageInfo{};
	skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyboxImageInfo.imageView = skyboxImageView;
	skyboxImageInfo.sampler = sampler;

	VkDescriptorImageInfo irradianceImageInfo{};
	irradianceImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	irradianceImageInfo.imageView = irradianceImageView;
	irradianceImageInfo.sampler = sampler;

	VkDescriptorImageInfo shadowMapImageInfo{};
	shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	shadowMapImageInfo.imageView = shadowMapImageView;
	shadowMapImageInfo.sampler = sampler;
	// Update the descriptor set with the new G-Buffer images


    std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_FinalPassDescriptorSets[frameIndex];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &diffuseImageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_FinalPassDescriptorSets[frameIndex];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &normalImageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &metallicRoughnessImageInfo;

	descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[3].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[3].dstBinding = 3;
	descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[3].descriptorCount = 1;
	descriptorWrites[3].pImageInfo = &depthImageInfo;

	descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[4].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[4].dstBinding = 4;
	descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[4].descriptorCount = 1;
	descriptorWrites[4].pBufferInfo = &bufferInfo;

	descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[5].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[5].dstBinding = 5;
	descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[5].descriptorCount = 1;
	descriptorWrites[5].pBufferInfo = &lightBufferInfo;

	descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[6].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[6].dstBinding = 6;
	descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[6].descriptorCount = 1;
	descriptorWrites[6].pImageInfo = &skyboxImageInfo;

	descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[7].dstSet = m_FinalPassDescriptorSets[frameIndex];
	descriptorWrites[7].dstBinding = 7;
	descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[7].descriptorCount = 1;
	descriptorWrites[7].pImageInfo = &irradianceImageInfo;

    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DescriptorManager::createComputeDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding inputImageBinding{};
    inputImageBinding.binding = 0;
    inputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    inputImageBinding.descriptorCount = 1;
    inputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputImageBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding outputImageBinding{};
    outputImageBinding.binding = 1;
    outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    outputImageBinding.descriptorCount = 1;
    outputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outputImageBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        inputImageBinding,
        outputImageBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_ComputeDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create compute descriptor set layout.");
    }
}

VkDescriptorSetLayout DescriptorManager::getComputeDescriptorSetLayout() const
{
	return m_ComputeDescriptorSetLayout;
}

void DescriptorManager::createComputeDescriptorSet(
    size_t frameIndex,
    VkImageView inputImageView,
    VkImageView outputImageView
)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_ComputeDescriptorSetLayout;

    if (vkAllocateDescriptorSets(m_Device, &allocInfo, &m_ComputeDescriptorSets[frameIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate compute descriptor set.");
    }

    VkDescriptorImageInfo inputImageInfo{};
    inputImageInfo.imageView = inputImageView;
    inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorImageInfo outputImageInfo{};
    outputImageInfo.imageView = outputImageView;
    outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_ComputeDescriptorSets[frameIndex];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &inputImageInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_ComputeDescriptorSets[frameIndex];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &outputImageInfo;

    vkUpdateDescriptorSets(
        m_Device,
        static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(),
        0,
        nullptr);
}

void DescriptorManager::updateComputeDescriptorSet(size_t frameIndex, VkImageView inputImageView, VkImageView outputImageView)
{
	VkDescriptorImageInfo inputImageInfo{};
	inputImageInfo.imageView = inputImageView;
	inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorImageInfo outputImageInfo{};
	outputImageInfo.imageView = outputImageView;
	outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;


	std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = m_ComputeDescriptorSets[frameIndex];
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pImageInfo = &inputImageInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = m_ComputeDescriptorSets[frameIndex];
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &outputImageInfo;

	vkUpdateDescriptorSets(
		m_Device,
		static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(),
		0,
		nullptr);
}

const std::vector<VkDescriptorSet>& DescriptorManager::getComputeDescriptorSets() const
{
	return m_ComputeDescriptorSets;
}
