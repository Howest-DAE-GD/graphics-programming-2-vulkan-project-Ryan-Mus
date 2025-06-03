#include "Renderer.h"
#include <stdexcept>
#include <array>
#include <chrono>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <spdlog/spdlog.h>

#include "Frustum.h"

Renderer::Renderer(Window* window)
    : m_pWindow(window),
      m_pCamera(nullptr)  // Initialize to nullptr
{
    spdlog::debug("Renderer created.");
}

Renderer::~Renderer() 
{
    cleanup();
    spdlog::debug("Destroying Renderer.");
}

void Renderer::initialize() 
{
    spdlog::debug("Initializing Renderer.");
    initVulkan();
    
    // Initialize the camera after Vulkan initialization
    m_pCamera = new Camera(
        m_pWindow->getGLFWwindow(),
        glm::vec3(0.0f, 0.0f, 0.3f),  // Camera position
        glm::vec3(0.0f, 1.0f, 0.0f),  // World up vector (Y-up)
        0.0f, 0.0f                    // Initial yaw and pitch
    );
    
    spdlog::debug("Renderer initialized.");
}

void Renderer::initVulkan() 
{
    m_pInstance = new Instance();

    m_pSurface = new Surface(m_pInstance->getInstance(), m_pWindow->getGLFWwindow());

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

	//Vulkan 1.1 features
	VkPhysicalDeviceVulkan11Features vulkan11Features{};
	vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

    //Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE;
	vulkan12Features.descriptorIndexing = VK_TRUE;

	//Vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features vulkan13Features{};
	vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13Features.synchronization2 = VK_TRUE;
	vulkan13Features.dynamicRendering = VK_TRUE;

    m_pPhysicalDevice = PhysicalDeviceBuilder()
        .setInstance(m_pInstance->getInstance())
        .setSurface(m_pSurface->get())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .addRequiredExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
        .addRequiredExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
		.addRequiredExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
		.addRequiredExtension(VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME)
        .setRequiredDeviceFeatures(deviceFeatures)
		.setVulkan11Features(vulkan11Features)
		.setVulkan12Features(vulkan12Features)
		.setVulkan13Features(vulkan13Features)
        .build();

    std::vector<const char*> validationLayers;
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    if (enableValidationLayers) 
    {
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        spdlog::debug("Validation layers enabled.");
    }

    m_pDevice = DeviceBuilder()
        .setPhysicalDevice(m_pPhysicalDevice->get())
        .setQueueFamilyIndices(m_pPhysicalDevice->getQueueFamilyIndices())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .addRequiredExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
        .addRequiredExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
		.addRequiredExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
//		.addRequiredExtension(VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME)
        .setEnabledFeatures(m_pPhysicalDevice->getFeatures())
		.setVulkan11Features(m_pPhysicalDevice->getVulkan11Features())
		.setVulkan12Features(m_pPhysicalDevice->getVulkan12Features())
		.setVulkan13Features(m_pPhysicalDevice->getVulkan13Features())
        .enableValidationLayers(validationLayers)
        .build();

    m_pSwapChain = SwapChainBuilder()
        .setDevice(m_pDevice->get())
        .setPhysicalDevice(m_pPhysicalDevice->get())
        .setSurface(m_pSurface->get())
        .setWidth(m_pWindow->getWidth())
        .setHeight(m_pWindow->getHeight())
        .setGraphicsFamilyIndex(m_pPhysicalDevice->getQueueFamilyIndices().graphicsFamily.value())
        .setPresentFamilyIndex(m_pPhysicalDevice->getQueueFamilyIndices().presentFamily.value())
        .setImageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) // Add VK_IMAGE_USAGE_TRANSFER_DST_BIT
        .build();

    //m_pRenderPass = new RenderPass(m_pDevice->get(), m_pSwapChain->getImageFormat(), findDepthFormat());
    createVmaAllocator();
    m_pCommandPool = new CommandPool(m_pDevice->get(), m_pPhysicalDevice->getQueueFamilyIndices().graphicsFamily.value());

	createGBuffer();

	createHDRImage();

	createLDRImage();

    createLightBuffer();

	m_Lights.push_back(Light{ glm::vec3(6.0f, 1.f, -0.2f), glm::vec3(1.f, 0.5f, 1.0f), 3.0f, 100.0f });

    createSunMatricesBuffers();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
		updateLightBuffer(i);
        updateSunMatricesBuffer(i);
    }

    m_pModel = new Model(m_VmaAllocator, m_pDevice, m_pPhysicalDevice, m_pCommandPool, MODEL_PATH_);
    m_pModel->loadModel();

    size_t materialCount = m_pModel->getMaterials().size();
    m_pDescriptorManager = new DescriptorManager(m_pDevice->get(), MAX_FRAMES_IN_FLIGHT, materialCount);

    createSkyboxCubeMap();
	createIrradianceMap();
    
    // Create descriptor set layouts and pools    
    m_pDescriptorManager->createDescriptorSetLayout();
    m_pDescriptorManager->createFinalPassDescriptorSetLayout();
	m_pDescriptorManager->createComputeDescriptorSetLayout();

    m_pDescriptorManager->createDescriptorPool();

    m_pModel->createVertexBuffer();
    m_pModel->createIndexBuffer();

    createUniformBuffers();

    // Gather uniform buffer handles
    std::vector<VkBuffer> uniformBufferHandles;
    for (const auto& buffer : m_pUniformBuffers) {
        uniformBufferHandles.push_back(buffer->get());
    }

    // Get materials from the Model
    const std::vector<Material*>& materials = m_pModel->getMaterials();

    // Create descriptor sets for the main pass
    m_pDescriptorManager->createDescriptorSets(
        uniformBufferHandles,
        materials,
        sizeof(UniformBufferObject)
    );

    // Create descriptor set for the final pass
    for (size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
    {
        m_pDescriptorManager->createFinalPassDescriptorSet(
            frameIndex,
            m_GBuffers[frameIndex].diffuseImageView,
            m_GBuffers[frameIndex].normalImageView,
            m_GBuffers[frameIndex].metallicRoughnessImageView,
            m_GBuffers[frameIndex].depthImageView,
            m_pUniformBuffers[frameIndex]->get(),
            sizeof(UniformBufferObject),
			m_pLightBuffers[frameIndex]->get(),
			sizeof(Light) * MAX_LIGHT_COUNT + sizeof(uint32_t),
			m_pSunMatricesBuffers[frameIndex]->get(),
			sizeof(SunMatricesUBO),
			m_GBuffers[frameIndex].shadowMapImageView, // Shadow map image view
			m_SkyboxCubeMapImageView, // Skybox cube map image view
			m_IrradianceMapImageView, // Irradiance map image view
            Texture::getTextureSampler() // Ensure this sampler is created
        );
    }

	// Create descriptor set for the compute pass
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_pDescriptorManager->createComputeDescriptorSet(
			i,
			m_HDRImageView[i],
			m_LDRImageView[i]
		);
    }

    createCommandBuffers();

    m_pGraphicsPipeline = GraphicsPipelineBuilder()
        .setDevice(m_pDevice->get())
        .setDescriptorSetLayout(m_pDescriptorManager->getDescriptorSetLayout())
        .setSwapChainExtent(m_pSwapChain->getExtent())
        .setColorFormats({ VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM}) // Multiple color formats
        .setDepthFormat(findDepthFormat())
        .setVertexInputBindingDescription(Vertex::getBindingDescription())
        .setVertexInputAttributeDescriptions(Vertex::getAttributeDescriptions())
        .setShaderPaths("shaders/shader.vert.spv", "shaders/shader.frag.spv")
		.setAttachmentCount(3)
		.enableDepthTest(true)
		.enableDepthWrite(false)
		.setDepthCompareOp(VK_COMPARE_OP_EQUAL)
        .build();

	m_pDepthPipeline = GraphicsPipelineBuilder()
        .setDevice(m_pDevice->get())
		.setDescriptorSetLayout(m_pDescriptorManager->getDescriptorSetLayout())
        .setSwapChainExtent(m_pSwapChain->getExtent())
        .setDepthFormat(findDepthFormat())
        .setVertexInputBindingDescription(Vertex::getBindingDescription())
        .setVertexInputAttributeDescriptions(Vertex::getDepthAttributeDescriptions())
        .setShaderPaths("shaders/depth.vert.spv", "shaders/depth.frag.spv")
        .setAttachmentCount(1)
        .enableDepthTest(true)
        .enableDepthWrite(true)
		.setDepthCompareOp(VK_COMPARE_OP_LESS)
        .build();

	//Create the shadow map pipeline
	m_pShadowMapPipeline = GraphicsPipelineBuilder()
		.setDevice(m_pDevice->get())
		.setDescriptorSetLayout(m_pDescriptorManager->getDescriptorSetLayout())
		.setSwapChainExtent(m_pSwapChain->getExtent())
		.setDepthFormat(findDepthFormat())
		.setVertexInputBindingDescription(Vertex::getBindingDescription())
		.setVertexInputAttributeDescriptions(Vertex::getDepthAttributeDescriptions())
		.setShaderPaths("shaders/shadow_map.vert.spv", "shaders/shadow_map.frag.spv")
		.setAttachmentCount(1)
		.enableDepthTest(true)
		.enableDepthWrite(true)
		.setDepthCompareOp(VK_COMPARE_OP_LESS)
        .setRasterizationState(VK_CULL_MODE_NONE)
		.setDepthBiasConstantFactor(1.25f) // Adjust as needed for shadow bias
		.setDepthBiasSlopeFactor(1.75f) // Adjust as needed for shadow bias
		.setPushConstantRange(sizeof(glm::mat4) * 2) // View and projection matrices
		.build();

	renderShadowMap();

    // Create the final pass graphics pipeline
    m_pFinalPipeline = GraphicsPipelineBuilder()
        .setDevice(m_pDevice->get())
        .setDescriptorSetLayout(m_pDescriptorManager->getFinalPassDescriptorSetLayout())
        .setSwapChainExtent(m_pSwapChain->getExtent())
        .setColorFormats({ VK_FORMAT_R32G32B32A32_SFLOAT })
        .setDepthFormat(VK_FORMAT_UNDEFINED)
        .setVertexInputBindingDescription({})
        .setVertexInputAttributeDescriptions({})
        .setShaderPaths("shaders/final.vert.spv", "shaders/final.frag.spv")
        .setAttachmentCount(1)
        .enableDepthTest(false)
        .setRasterizationState(VK_CULL_MODE_NONE)
        .setPushConstantRange(sizeof(DebugPushConstants))
		.setPushConstantFlags(VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

	m_pToneMappingPipeline = ComputePipelineBuilder()
		.setDevice(m_pDevice)
		.setShaderPath("shaders/tone_mapping.comp.spv")
		.setDescriptorSetLayout(m_pDescriptorManager->getComputeDescriptorSetLayout())
		.build();

    m_pSyncObjects = new SynchronizationObjects(m_pDevice->get(), MAX_FRAMES_IN_FLIGHT);

    transitionSwapchainImagesToPresentLayout();
}

void Renderer::createVmaAllocator() 
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_pPhysicalDevice->get();
    allocatorInfo.device = m_pDevice->get();
    allocatorInfo.instance = m_pInstance->getInstance();
    vmaCreateAllocator(&allocatorInfo, &m_VmaAllocator);
}

VkFormat Renderer::findDepthFormat() 
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Renderer::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) 
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_pPhysicalDevice->get(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

void Renderer::renderToCubeMap(
    Image* pInputImage,
    VkImageView inputImageView,
    Image* pOutputCubeMapImage,
    std::array<VkImageView, 6> outputCubeMapImageViews,
    VkSampler sampler,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath
)
{
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(m_pDevice->get(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    // Create push constant range
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4) * 2; // View and projection matrices

    // Create graphics pipeline
    GraphicsPipeline* pGraphicsPipeline = GraphicsPipelineBuilder()
        .setDevice(m_pDevice->get())
		.setDescriptorSetLayout(descriptorSetLayout)
		.setPushConstantRange(sizeof(glm::mat4) * 2)
        .setVertexInputBindingDescription({})
        .setVertexInputAttributeDescriptions({})
        .setShaderPaths(vertexShaderPath, fragmentShaderPath)
        .setColorFormats({ VK_FORMAT_R32G32B32A32_SFLOAT })
        .setDepthFormat(VK_FORMAT_UNDEFINED)
        .setRasterizationState(VK_CULL_MODE_NONE)
        .setAttachmentCount(1)
        .enableDepthTest(false)
        .enableDepthWrite(false)
        .build();

    // Create descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(m_pDevice->get(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(m_pDevice->get(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }

    // Update descriptor set
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = pInputImage->getImageLayout();
    imageInfo.imageView = inputImageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_pDevice->get(), 1, &descriptorWrite, 0, nullptr);

    // Begin command buffer
    VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();

    // Set up matrices
    glm::vec3 eye = glm::vec3(0.0f);
    glm::mat4 captureViews[6] = {
        glm::lookAt(eye, eye + glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
        glm::lookAt(eye, eye + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
        glm::lookAt(eye, eye + glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
        glm::lookAt(eye, eye + glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y
        glm::lookAt(eye, eye + glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
        glm::lookAt(eye, eye + glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z
    };

    glm::mat4 captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    captureProj[1][1] *= -1.0f; // Inverted Y-axis for matching face orientation

    // Loop over each cube map face
    for (uint32_t face = 0; face < 6; ++face)
    {
        // Transition cube map face to color attachment optimal
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = face;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // <-- This might be incorrect
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.srcAccessMask = VK_ACCESS_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.image = pOutputCubeMapImage->getImage();
        barrier.subresourceRange = subresourceRange;

        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

        // Begin dynamic rendering
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = outputCubeMapImageViews[face];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { {0.0f, 0.0f, 0.0f, 1.0f} };

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = { pOutputCubeMapImage->getWidth(), pOutputCubeMapImage->getHeight() };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        // Set viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(pOutputCubeMapImage->getWidth());
        viewport.height = static_cast<float>(pOutputCubeMapImage->getHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { pOutputCubeMapImage->getWidth(), pOutputCubeMapImage->getHeight() };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind pipeline and descriptor sets
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pGraphicsPipeline->get());

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pGraphicsPipeline->getPipelineLayout(),
            0,
            1,
            &descriptorSet,
            0,
            nullptr
        );

        // Push constants for view and projection matrices
        struct PushConstants {
            glm::mat4 view;
            glm::mat4 proj;
        } pushConstants;

        pushConstants.view = captureViews[face];
        pushConstants.proj = captureProj;

        vkCmdPushConstants(
            commandBuffer,
            pGraphicsPipeline->getPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstants),
            &pushConstants
        );

        // Draw call
        vkCmdDraw(commandBuffer, 36, 1, 0, 0); // Draw all 36 vertices for the full cube

        vkCmdEndRendering(commandBuffer);

        // Transition cube map face to shader read optimal
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;


        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    }

    // End command buffer and submit
    m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());

    // Cleanup
    delete pGraphicsPipeline;
    vkDestroyDescriptorPool(m_pDevice->get(), descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_pDevice->get(), descriptorSetLayout, nullptr);
}


void Renderer::createUniformBuffers() 
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    m_pUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_pUniformBuffers[i] = new Buffer(
            m_VmaAllocator,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
    }
}

void Renderer::createLightBuffer()
{
    VkDeviceSize bufferSize = sizeof(uint32_t) + sizeof(Light) * MAX_LIGHT_COUNT;
	m_pLightBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_pLightBuffers[i] = new Buffer(
            m_VmaAllocator,
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
    }
}

void Renderer::createSunMatricesBuffers()
{
    VkDeviceSize bufferSize = sizeof(SunMatricesUBO);
    m_pSunMatricesBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_pSunMatricesBuffers[i] = new Buffer(
            m_VmaAllocator,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
    }
}

void Renderer::createCommandBuffers() 
{
    m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_pCommandPool->get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    if (vkAllocateCommandBuffers(m_pDevice->get(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Renderer::createSkyboxCubeMap()
{
    // **1. Load HDRI Image using stbi_loadf**
    int texWidth, texHeight, texChannels;
    float* pixels = stbi_loadf(HDRI_PATH_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load HDRI texture!");
    }
    VkDeviceSize imageSize = texWidth * texHeight * 4 * sizeof(float);

    // **2. Create Staging Buffer and Copy Data**
    Buffer stagingBuffer(
        m_VmaAllocator,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    );

    void* data = stagingBuffer.map();
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    stagingBuffer.unmap();

    stbi_image_free(pixels);

    // **3. Create HDRI Image**
    Image* pHDRIImage = new Image(m_pDevice, m_VmaAllocator);
    pHDRIImage->createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R32G32B32A32_SFLOAT, // Floating point format for HDR
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // **4. Copy Data from Staging Buffer to HDRI Image**
    {
        VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
        transitionImageLayout(
            commandBuffer,
            pHDRIImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            0,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
    }

    pHDRIImage->copyBufferToImage(
        m_pCommandPool,
        stagingBuffer.get(),
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    {
        VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
        transitionImageLayout(
            commandBuffer,
            pHDRIImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
    }

    // **5. Create Image View for HDRI Image**
    VkImageView pHDRIImageView = pHDRIImage->createImageView(
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // **6. Create Cube Map Image**
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.extent.width = 1024;
    imageInfo.extent.height = 1024;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    m_pSkyboxCubeMapImage = new Image(m_pDevice, m_VmaAllocator);
    m_pSkyboxCubeMapImage->createImage(
        imageInfo.extent.width,
        imageInfo.extent.height,
        imageInfo.format,
        imageInfo.tiling,
        imageInfo.usage,
        imageInfo.flags,
        imageInfo.arrayLayers,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // **7. Create Image Views for Each Face of the Cube Map**
    for (uint32_t face = 0; face < 6; ++face)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_pSkyboxCubeMapImage->getImage();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageInfo.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = face;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_pDevice->get(), &viewInfo, nullptr, &m_SkyboxCubeMapImageViews[face]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views for cube map faces!");
        }
    }

    {
        VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
        transitionImageLayout(
            commandBuffer,
            m_pSkyboxCubeMapImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
    }

    // **8. Render to Cube Map**
    renderToCubeMap(
        pHDRIImage,
        pHDRIImageView,
        m_pSkyboxCubeMapImage,
        m_SkyboxCubeMapImageViews,
        Texture::getTextureSampler(),
        "shaders/skybox.vert.spv",
        "shaders/skybox.frag.spv"
    );

    // **9. Cleanup**
    vkDestroyImageView(m_pDevice->get(), pHDRIImageView, nullptr);
    delete pHDRIImage;

    for (uint32_t face = 0; face < 6; ++face)
    {
        vkDestroyImageView(m_pDevice->get(), m_SkyboxCubeMapImageViews[face], nullptr);
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_pSkyboxCubeMapImage->getImage();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

	if (vkCreateImageView(m_pDevice->get(), &viewInfo, nullptr, &m_SkyboxCubeMapImageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image view for cube map!");
	}

    // Transition Skybox Cube Map to SHADER_READ_ONLY_OPTIMAL
    {
        VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
        transitionImageLayout(
            commandBuffer,
            m_pSkyboxCubeMapImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            0,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
    }
}

void Renderer::createIrradianceMap()
{
    // Create the irradiance map image
    m_pIrradianceMapImage = new Image(m_pDevice, m_VmaAllocator);
    m_pIrradianceMapImage->createImage(
        64,
        64,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        6,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // Transition Irradiance Map to COLOR_ATTACHMENT_OPTIMAL for rendering
    {
        VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
        transitionImageLayout(
            commandBuffer,
            m_pIrradianceMapImage,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
        m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
    }

	for (int face = 0; face < 6; ++face)
	{
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_pIrradianceMapImage->getImage();
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = face;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_pDevice->get(), &viewInfo, nullptr, &m_IrradianceMapImageViews[face]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views for cube map faces!");
        }
	}

    // Render to Cube Map
    renderToCubeMap(
        m_pSkyboxCubeMapImage,
        m_SkyboxCubeMapImageView,
        m_pIrradianceMapImage,
        m_IrradianceMapImageViews,
        Texture::getTextureSampler(),
        "shaders/skybox.vert.spv",
        "shaders/irradiance.frag.spv"
    );

	// Cleanup
	for (int face = 0; face < 6; ++face)
	{
		vkDestroyImageView(m_pDevice->get(), m_IrradianceMapImageViews[face], nullptr);
	}

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_pIrradianceMapImage->getImage();
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(m_pDevice->get(), &viewInfo, nullptr, &m_IrradianceMapImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view for cube map!");
    }
}

void Renderer::renderShadowMap()
{
    auto [aabbMin, aabbMax] = m_pModel->getAABB();
    glm::vec3 sceneCenter = (aabbMin + aabbMax) * 0.5f;
    glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.4f));

    std::vector<glm::vec3> corners = {
        {aabbMin.x, aabbMin.y, aabbMin.z},
        {aabbMax.x, aabbMin.y, aabbMin.z},
        {aabbMin.x, aabbMax.y, aabbMin.z},
        {aabbMax.x, aabbMax.y, aabbMin.z},
        {aabbMin.x, aabbMin.y, aabbMax.z},
        {aabbMax.x, aabbMin.y, aabbMax.z},
        {aabbMin.x, aabbMax.y, aabbMax.z},
        {aabbMax.x, aabbMax.y, aabbMax.z},
    };

    float minProj = std::numeric_limits<float>::max();
    float maxProj = std::numeric_limits<float>::lowest();
    for (const glm::vec3& corner : corners) {
        float proj = glm::dot(corner, lightDirection);
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }

    float distance = maxProj - glm::dot(sceneCenter, lightDirection);
    glm::vec3 lightPos = sceneCenter - lightDirection * distance * 2.f;
    glm::vec3 up = glm::abs(glm::dot(lightDirection, glm::vec3(0.f, 1.f, 0.f))) > 0.99f
        ? glm::vec3(0.f, 0.f, 1.f)
        : glm::vec3(0.f, 1.f, 0.f);

    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, up);

    glm::vec3 minLS(std::numeric_limits<float>::max());
    glm::vec3 maxLS(-std::numeric_limits<float>::max());
    for (const glm::vec3& corner : corners) {
        glm::vec3 tr = glm::vec3(lightView * glm::vec4(corner, 1.0f));
        minLS = glm::min(minLS, tr);
        maxLS = glm::max(maxLS, tr);
    }

    float nearZ = 0.000f;
    float farZ = (maxLS.z - minLS.z) * 1.5f;
    glm::mat4 lightProj = glm::ortho(
        minLS.x, maxLS.x,
        minLS.y, maxLS.y,
        nearZ, farZ
    );
    lightProj[1][1] *= -1.0f;

    // Store the matrices in the Renderer class
    m_LightProj = lightProj;
    m_LightView = lightView;

    // 2. For each frame in flight, render the shadow map
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        // Begin single time command buffer (or use your frame's command buffer if you want to batch)
        VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();

        // Transition shadow map image to depth attachment optimal
        transitionImageLayout(
            commandBuffer,
            m_GBuffers[i].pShadowMapImage,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );

        // Set up rendering info
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = m_GBuffers[i].shadowMapImageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = { m_GBuffers[i].pShadowMapImage->getWidth(), m_GBuffers[i].pShadowMapImage->getHeight() };
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 0;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        // Bind shadow map pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pShadowMapPipeline->get());

        // Bind vertex and index buffers
        VkBuffer vertexBuffers[] = { m_pModel->getVertexBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_pModel->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Set viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_GBuffers[i].pShadowMapImage->getWidth());
        viewport.height = static_cast<float>(m_GBuffers[i].pShadowMapImage->getHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { m_GBuffers[i].pShadowMapImage->getWidth(), m_GBuffers[i].pShadowMapImage->getHeight() };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Push constants or bind UBO for lightView and lightProj as needed by your shadow map shaders
        struct ShadowPushConstants {
            glm::mat4 lightView;
            glm::mat4 lightProj;
        } shadowPC;
        shadowPC.lightView = lightView;
        shadowPC.lightProj = lightProj;

        vkCmdPushConstants(
            commandBuffer,
            m_pShadowMapPipeline->getPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(ShadowPushConstants),
            &shadowPC
        );

        // Draw all submeshes
        vkCmdDrawIndexed(
            commandBuffer,
            m_pModel->getIndexCount(),
            1,
            0,
            0,
            0
        );
      
        vkCmdEndRendering(commandBuffer);

        // Transition shadow map image to shader read optimal for later use
        transitionImageLayout(
            commandBuffer,
            m_GBuffers[i].pShadowMapImage,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );
        // End and submit
        m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
    }    
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    // Get the G-buffer for the current frame
    GBuffer& currentGBuffer = m_GBuffers[m_currentFrame];

    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // Transition depth image to DEPTH_STENCIL_ATTACHMENT_OPTIMAL for depth pre-pass
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDepthImage,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // Prepare common variables
    VkBuffer vertexBuffers[] = { m_pModel->getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_pSwapChain->getExtent().width);
    viewport.height = static_cast<float>(m_pSwapChain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_pSwapChain->getExtent();

    Frustum frustum{ m_UniformBufferObject.proj, m_UniformBufferObject.view };
    const auto& submeshes = m_pModel->getSubmeshes();

    // **Depth Pre-Pass**
    {
        // Begin depth-only rendering
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = currentGBuffer.depthImageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth buffer
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

        VkRenderingInfo depthRenderingInfo{};
        depthRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        depthRenderingInfo.renderArea.offset = { 0, 0 };
        depthRenderingInfo.renderArea.extent = m_pSwapChain->getExtent();
        depthRenderingInfo.layerCount = 1;
        depthRenderingInfo.colorAttachmentCount = 0; // No color attachments
        depthRenderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(commandBuffer, &depthRenderingInfo);

        // Bind the depth pre-pass pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pDepthPipeline->get());

        // Bind vertex and index buffers
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_pModel->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Set viewport and scissor
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Render the scene
        for (const auto& submesh : submeshes)
        {
            // Transform the bounding box by the model matrix
            glm::vec3 transformedMin = glm::vec3(m_UniformBufferObject.model * glm::vec4(submesh.bboxMin, 1.0f));
            glm::vec3 transformedMax = glm::vec3(m_UniformBufferObject.model * glm::vec4(submesh.bboxMax, 1.0f));

            // Perform frustum culling
            if (!frustum.isBoxVisible(transformedMin, transformedMax)) {
                continue;
            }

            // Calculate descriptor set index
            uint32_t descriptorSetIndex = m_currentFrame * m_pModel->getMaterials().size() + submesh.materialIndex;

            // Bind descriptor set for the current material and frame
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pGraphicsPipeline->getPipelineLayout(),
                0,
                1,
                &m_pDescriptorManager->getDescriptorSets()[descriptorSetIndex],
                0,
                nullptr
            );

            // Draw submesh
            vkCmdDrawIndexed(
                commandBuffer,
                submesh.indexCount,
                1,
                submesh.indexStart,
                0,
                0
            );
        }

        vkCmdEndRendering(commandBuffer);
    }
           // Ensure depth data is available for the main pass
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDepthImage,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // Transition G-buffer images for the current frame
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDiffuseImage,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pNormalImage,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pMetallicRougnessImage,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // **Main Rendering Pass**
    {
        // Set up color attachments
        VkRenderingAttachmentInfo colorAttachments[3]{};

        // Diffuse attachment
        colorAttachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[0].imageView = currentGBuffer.diffuseImageView;
        colorAttachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[0].clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        // Normal attachment
        colorAttachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[1].imageView = currentGBuffer.normalImageView;
        colorAttachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[1].clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        // Metallic-Roughness attachment
        colorAttachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[2].imageView = currentGBuffer.metallicRoughnessImageView;
        colorAttachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[2].clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        // Depth attachment (load existing depth buffer from pre-pass)
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = currentGBuffer.depthImageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load depth from pre-pass
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = m_pSwapChain->getExtent();
        renderingInfo.layerCount = 1;
        renderingInfo.viewMask = 0;
        renderingInfo.colorAttachmentCount = 3;
        renderingInfo.pColorAttachments = colorAttachments;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->get());

        // Bind vertex and index buffers
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_pModel->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Set viewport and scissor
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Render the scene with material bindings
        for (const auto& submesh : submeshes)
        {
            // Transform the bounding box by the model matrix
            glm::vec3 transformedMin = glm::vec3(m_UniformBufferObject.model * glm::vec4(submesh.bboxMin, 1.0f));
            glm::vec3 transformedMax = glm::vec3(m_UniformBufferObject.model * glm::vec4(submesh.bboxMax, 1.0f));

            // Perform frustum culling
            if (!frustum.isBoxVisible(transformedMin, transformedMax)) {
                continue;
            }

            // Calculate descriptor set index
            uint32_t descriptorSetIndex = m_currentFrame * m_pModel->getMaterials().size() + submesh.materialIndex;

            // Bind descriptor set for the current material and frame
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pGraphicsPipeline->getPipelineLayout(),
                0,
                1,
                &m_pDescriptorManager->getDescriptorSets()[descriptorSetIndex],
                0,
                nullptr
            );

            // Draw submesh
            vkCmdDrawIndexed(
                commandBuffer,
                submesh.indexCount,
                1,
                submesh.indexStart,
                0,
                0
            );
        }

        vkCmdEndRendering(commandBuffer);
    }

    // Define target layouts for post-rendering
    VkImageLayout targetColorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkImageLayout targetDepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    // Transition G-buffer images to SHADER_READ_ONLY_OPTIMAL for final pass
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDiffuseImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        targetColorLayout,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pNormalImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        targetColorLayout,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pMetallicRougnessImage,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        targetColorLayout,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Transition depth image to DEPTH_STENCIL_READ_ONLY_OPTIMAL for final pass
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDepthImage,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        targetDepthLayout,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

	// Transition HDR image to write layout for tone mapping
	transitionImageLayout(
		commandBuffer,
		m_pHDRImage[m_currentFrame],
		m_pHDRImage[m_currentFrame]->getImageLayout(),
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_ACCESS_2_SHADER_READ_BIT,
		VK_ACCESS_2_SHADER_WRITE_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);   

    // **Final Rendering Pass**
    {
        // Begin final rendering pass to the swapchain image
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_HDRImageView[m_currentFrame];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

        VkRenderingInfo finalRenderingInfo{};
        finalRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        finalRenderingInfo.renderArea.offset = { 0, 0 };
        finalRenderingInfo.renderArea.extent = m_pSwapChain->getExtent();
        finalRenderingInfo.layerCount = 1;
        finalRenderingInfo.colorAttachmentCount = 1;
        finalRenderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &finalRenderingInfo);

        // Bind the final pass pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pFinalPipeline->get());

        // Bind the descriptor set with G-buffer images
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pFinalPipeline->getPipelineLayout(),
            0,
            1,
            &m_pDescriptorManager->getFinalPassDescriptorSets()[m_currentFrame],
            0,
            nullptr
        );
        // Update debug push constants with camera's debug settings and intensity values
        m_DebugPushConstants.debugMode = m_pCamera->getDebugMode();
        m_DebugPushConstants.iblIntensity = m_pCamera->getIblIntensity();
        m_DebugPushConstants.sunIntensity = m_pCamera->getSunIntensity();

        vkCmdPushConstants(
            commandBuffer,
            m_pFinalPipeline->getPipelineLayout(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(DebugPushConstants),
            &m_DebugPushConstants
        );

        // Set viewport and scissor
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Draw fullscreen triangle (or quad)
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }

    // Transition HDR image to GENERAL layout for compute shader read
    transitionImageLayout(
        commandBuffer,
        m_pHDRImage[m_currentFrame],
        m_pHDRImage[m_currentFrame]->getImageLayout(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Transition swapchain image to GENERAL layout for compute shader write
    transitionImageLayout(
        commandBuffer,
        m_pLDRImage[m_currentFrame],
        m_pLDRImage[m_currentFrame]->getImageLayout(),
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Bind the compute pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pToneMappingPipeline->getPipeline());

    // Bind the descriptor set for the compute shader
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        m_pToneMappingPipeline->getPipelineLayout(),
        0,
        1,
        &m_pDescriptorManager->getComputeDescriptorSets()[m_currentFrame],
        0,
        nullptr
    );

    // Dispatch the compute shader
    uint32_t workgroupSizeX = 16;
    uint32_t workgroupSizeY = 16;
    uint32_t dispatchX = (m_pSwapChain->getExtent().width + workgroupSizeX - 1) / workgroupSizeX;
    uint32_t dispatchY = (m_pSwapChain->getExtent().height + workgroupSizeY - 1) / workgroupSizeY;

    vkCmdDispatch(commandBuffer, dispatchX, dispatchY, 1);

	blitLDRToSwapchain(m_currentFrame, commandBuffer);

    // End command buffer recording
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void Renderer::drawFrame()
{
    vkWaitForFences(m_pDevice->get(), 1, m_pSyncObjects->getInFlightFence(m_currentFrame), VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_pDevice->get(),
        m_pSwapChain->get(),
        UINT64_MAX,
        *m_pSyncObjects->getImageAvailableSemaphore(m_currentFrame),
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_pDevice->get(), 1, m_pSyncObjects->getInFlightFence(m_currentFrame));

    vkResetCommandBuffer(m_CommandBuffers[m_currentFrame], 0);
    recordCommandBuffer(m_CommandBuffers[m_currentFrame], imageIndex);

    updateUniformBuffer(m_currentFrame);
    updateSunMatricesBuffer(m_currentFrame);

    // Prepare VkCommandBufferSubmitInfo
    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.pNext = nullptr;
    commandBufferInfo.commandBuffer = m_CommandBuffers[m_currentFrame];
    commandBufferInfo.deviceMask = 0;

    // Prepare VkSemaphoreSubmitInfo for wait semaphore
    VkSemaphoreSubmitInfo waitSemaphoreInfo{};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.pNext = nullptr;
    waitSemaphoreInfo.semaphore = *m_pSyncObjects->getImageAvailableSemaphore(m_currentFrame);
    waitSemaphoreInfo.value = 0;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    waitSemaphoreInfo.deviceIndex = 0;

    // Prepare VkSemaphoreSubmitInfo for signal semaphore
    VkSemaphoreSubmitInfo signalSemaphoreInfo{};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.pNext = nullptr;
    signalSemaphoreInfo.semaphore = *m_pSyncObjects->getRenderFinishedSemaphore(m_currentFrame);
    signalSemaphoreInfo.value = 0;
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    signalSemaphoreInfo.deviceIndex = 0;

    // Prepare VkSubmitInfo2
    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.pNext = nullptr;
    submitInfo.flags = 0;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    // Submit the command buffer using vkQueueSubmit2
    if (vkQueueSubmit2(m_pDevice->getGraphicsQueue(), 1, &submitInfo, *m_pSyncObjects->getInFlightFence(m_currentFrame)) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // **Updated Section: Include the RenderFinishedSemaphore in vkQueuePresentKHR**
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Wait on the RenderFinishedSemaphore before presenting
    VkSemaphore waitSemaphores[] = { *m_pSyncObjects->getRenderFinishedSemaphore(m_currentFrame) };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;

    VkSwapchainKHR swapChains[] = { m_pSwapChain->get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_pDevice->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_pWindow->isFramebufferResized()) {
        m_pWindow->resetFramebufferResized();
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::updateUniformBuffer(uint32_t currentImage)
{
    // Calculate delta time
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;

    // Update the camera (now using member variable)
    m_pCamera->update(deltaTime);

    m_UniformBufferObject.model = glm::mat4(1.0f);
    m_UniformBufferObject.view = m_pCamera->getViewMatrix();
    m_UniformBufferObject.proj = glm::perspective(
        glm::radians(45.0f),
        m_pSwapChain->getExtent().width / (float)m_pSwapChain->getExtent().height,
        0.001f,
        100.0f);
    m_UniformBufferObject.proj[1][1] *= -1;

    // Set the camera position
    m_UniformBufferObject.cameraPosition = m_pCamera->getPosition();
    m_UniformBufferObject.viewportSize = glm::vec2(m_pSwapChain->getExtent().width, m_pSwapChain->getExtent().height);

    // Map the uniform buffer and copy the data
    void* data = m_pUniformBuffers[currentImage]->map();
    memcpy(data, &m_UniformBufferObject, sizeof(m_UniformBufferObject));
}

void Renderer::updateLightBuffer(uint32_t currentImage)
{
    struct LightsBuffer {
        uint32_t lightCount;
        Light lights[MAX_LIGHT_COUNT];
    } lightsBufferData;

    lightsBufferData.lightCount = static_cast<uint32_t>(m_Lights.size());
    memcpy(lightsBufferData.lights, m_Lights.data(), sizeof(Light) * m_Lights.size());

    void* data = m_pLightBuffers[currentImage]->map();
    memcpy(data, &lightsBufferData, sizeof(lightsBufferData));
    m_pLightBuffers[currentImage]->unmap();
}

void Renderer::updateSunMatricesBuffer(uint32_t currentImage)
{
    SunMatricesUBO sunMatrices;
    sunMatrices.lightProj = m_LightProj;
    sunMatrices.lightView = m_LightView;
    void* data = m_pSunMatricesBuffers[currentImage]->map();
    memcpy(data, &sunMatrices, sizeof(sunMatrices));
    m_pSunMatricesBuffers[currentImage]->unmap();
}

void Renderer::recreateSwapChain()
{
    int width = 0, height = 0;
    m_pWindow->getFramebufferSize(width, height);
    while (width == 0 || height == 0) {
        m_pWindow->getFramebufferSize(width, height);
        m_pWindow->pollEvents();
    }

    vkDeviceWaitIdle(m_pDevice->get());

    cleanupSwapChain();

    m_pSwapChain = SwapChainBuilder()
        .setDevice(m_pDevice->get())
        .setPhysicalDevice(m_pPhysicalDevice->get())
        .setSurface(m_pSurface->get())
        .setWidth(width)
        .setHeight(height)
        .setGraphicsFamilyIndex(m_pPhysicalDevice->getQueueFamilyIndices().graphicsFamily.value())
        .setPresentFamilyIndex(m_pPhysicalDevice->getQueueFamilyIndices().presentFamily.value())
        .setImageUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build();

    transitionSwapchainImagesToPresentLayout();

    createGBuffer();
	createHDRImage();
	createLDRImage();

    m_pSwapChain->getImages();


    // Update descriptor sets and swapvhain
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_pDescriptorManager->updateFinalPassDescriptorSet(
            i,
            m_GBuffers[i].diffuseImageView,
            m_GBuffers[i].normalImageView,
            m_GBuffers[i].metallicRoughnessImageView,
            m_GBuffers[i].depthImageView,
            m_pUniformBuffers[i]->get(),
            sizeof(UniformBufferObject),
            m_pLightBuffers[i]->get(),
            (sizeof(Light) * MAX_LIGHT_COUNT + sizeof(uint32_t)),
			m_pSunMatricesBuffers[i]->get(),
			sizeof(SunMatricesUBO),
			m_GBuffers[i].shadowMapImageView,
			m_SkyboxCubeMapImageView,
            m_IrradianceMapImageView,
            Texture::getTextureSampler()
        );

		m_pDescriptorManager->updateComputeDescriptorSet(
			i,
			m_HDRImageView[i],
			m_LDRImageView[i]
		);
    }

    // Re-render the shadow map to match the new swapchain resolution
    renderShadowMap();
}

void Renderer::transitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 srcAccessMask,
    VkAccessFlags2 dstAccessMask,
    VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcStageMask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void Renderer::transitionImageLayout(VkCommandBuffer commandBuffer,
    Image* pImage,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask,
    VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 srcAccessMask,
    VkAccessFlags2 dstAccessMask,
    VkImageAspectFlags aspectMask)
{
	transitionImageLayout(
		commandBuffer,
		pImage->getImage(),
		oldLayout,
		newLayout,
		srcStageMask,
		dstStageMask,
		srcAccessMask,
		dstAccessMask,
		aspectMask
	);
	pImage->setImageLayout(newLayout);
}

void Renderer::transitionSwapchainImagesToPresentLayout()
{
    VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		transitionImageLayout(
			commandBuffer,
			m_pSwapChain->getImages()[i],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_2_NONE,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}

    m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
}

void Renderer::createGBuffer()
{
    m_GBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        // Create diffuse image
        m_GBuffers[i].pDiffuseImage = new Image(m_pDevice, m_VmaAllocator);
        m_GBuffers[i].pDiffuseImage->createImage(m_pSwapChain->getExtent().width,
            m_pSwapChain->getExtent().height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        m_GBuffers[i].diffuseImageView = m_GBuffers[i].pDiffuseImage->createImageView(
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_GBuffers[i].pDiffuseImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                0,
                VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }
          
        // Create normal image
        m_GBuffers[i].pNormalImage = new Image(m_pDevice, m_VmaAllocator);
        m_GBuffers[i].pNormalImage->createImage(m_pSwapChain->getExtent().width,
            m_pSwapChain->getExtent().height,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        m_GBuffers[i].normalImageView = m_GBuffers[i].pNormalImage->createImageView(
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_GBuffers[i].pNormalImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                0,
                VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }

        // Create metallic-roughness image
        m_GBuffers[i].pMetallicRougnessImage = new Image(m_pDevice, m_VmaAllocator);
        m_GBuffers[i].pMetallicRougnessImage->createImage(m_pSwapChain->getExtent().width,
            m_pSwapChain->getExtent().height,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        m_GBuffers[i].metallicRoughnessImageView = m_GBuffers[i].pMetallicRougnessImage->createImageView(
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_GBuffers[i].pMetallicRougnessImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                0,
                VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }

        // Create depth image
        VkFormat depthFormat = findDepthFormat();
        m_GBuffers[i].pDepthImage = new Image(m_pDevice, m_VmaAllocator);
        m_GBuffers[i].pDepthImage->createImage(m_pSwapChain->getExtent().width,
            m_pSwapChain->getExtent().height,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        m_GBuffers[i].depthImageView = m_GBuffers[i].pDepthImage->createImageView(
            depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_GBuffers[i].pDepthImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                0,
                VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }

		// Create Shadow map image
		m_GBuffers[i].pShadowMapImage = new Image(m_pDevice, m_VmaAllocator);
		m_GBuffers[i].pShadowMapImage->createImage(
            2048,
			2048,
            depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		m_GBuffers[i].shadowMapImageView = m_GBuffers[i].pShadowMapImage->createImageView(
			depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_GBuffers[i].pShadowMapImage,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                0,
                VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }
    }
}

void Renderer::createHDRImage()
{
    m_pHDRImage.resize(MAX_FRAMES_IN_FLIGHT);
    m_HDRImageView.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i{}; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_pHDRImage[i] = new Image(m_pDevice, m_VmaAllocator);
        m_pHDRImage[i]->createImage(
            m_pSwapChain->getExtent().width,
            m_pSwapChain->getExtent().height,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        m_HDRImageView[i] = m_pHDRImage[i]->createImageView(
            VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_pHDRImage[i],
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                0,
                VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }
    }
}

void Renderer::createLDRImage()
{
	m_pLDRImage.resize(MAX_FRAMES_IN_FLIGHT);
	m_LDRImageView.resize(MAX_FRAMES_IN_FLIGHT);
	for (int i{}; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		m_pLDRImage[i] = new Image(m_pDevice, m_VmaAllocator);
		m_pLDRImage[i]->createImage(
			m_pSwapChain->getExtent().width,
			m_pSwapChain->getExtent().height,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);
		m_LDRImageView[i] = m_pLDRImage[i]->createImageView(
			VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        {
            VkCommandBuffer commandBuffer = m_pCommandPool->beginSingleTimeCommands();
            transitionImageLayout(
                commandBuffer,
                m_pLDRImage[i],
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                0,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT
            );
            m_pCommandPool->endSingleTimeCommands(commandBuffer, m_pDevice->getGraphicsQueue());
        }
	}
}

void Renderer::cleanupSwapChain()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyImageView(m_pDevice->get(), m_GBuffers[i].depthImageView, nullptr);
        delete m_GBuffers[i].pDepthImage;

        vkDestroyImageView(m_pDevice->get(), m_GBuffers[i].diffuseImageView, nullptr);
        delete m_GBuffers[i].pDiffuseImage;

        vkDestroyImageView(m_pDevice->get(), m_GBuffers[i].normalImageView, nullptr);
        delete m_GBuffers[i].pNormalImage;

        vkDestroyImageView(m_pDevice->get(), m_GBuffers[i].metallicRoughnessImageView, nullptr);
        delete m_GBuffers[i].pMetallicRougnessImage;

		vkDestroyImageView(m_pDevice->get(), m_GBuffers[i].shadowMapImageView, nullptr);
		delete m_GBuffers[i].pShadowMapImage;

		vkDestroyImageView(m_pDevice->get(), m_HDRImageView[i], nullptr);
		delete m_pHDRImage[i];

		vkDestroyImageView(m_pDevice->get(), m_LDRImageView[i], nullptr);
		delete m_pLDRImage[i];
    }

    // Clear the vector
    m_GBuffers.clear();

    delete m_pSwapChain;
}

void Renderer::blitLDRToSwapchain(uint32_t imageIndex, VkCommandBuffer commandBuffer)
{
    // Transition LDR image layout to transfer source
        // Transition LDR image layout to transfer source
    transitionImageLayout(
        commandBuffer,
        m_pLDRImage[imageIndex],
        m_pLDRImage[imageIndex]->getImageLayout(),                       // From compute shader
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,          // To transfer source
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,        // After compute shader
        VK_PIPELINE_STAGE_2_TRANSFER_BIT,              // Before transfer operations
        VK_ACCESS_2_SHADER_WRITE_BIT,                  // After compute writes
        VK_ACCESS_2_TRANSFER_READ_BIT,                 // For transfer read
        VK_IMAGE_ASPECT_COLOR_BIT);

    // **Modified: Transition swapchain image layout to transfer destination**
    transitionImageLayout(
        commandBuffer,
        m_pSwapChain->getImages()[imageIndex],
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,               // **From present source**
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,          // To transfer destination
        VK_PIPELINE_STAGE_2_TRANSFER_BIT,              // Using transfer stages
        VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_NONE,                              // No prior access
        VK_ACCESS_2_TRANSFER_WRITE_BIT,                // For transfer write
        VK_IMAGE_ASPECT_COLOR_BIT);


    // Blit the image
    VkImageBlit blitRegion{};
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcOffsets[0] = { 0, 0, 0 };
    blitRegion.srcOffsets[1] = {
        static_cast<int32_t>(m_pSwapChain->getExtent().width),
        static_cast<int32_t>(m_pSwapChain->getExtent().height),
        1
    };
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.mipLevel = 0;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstOffsets[0] = { 0, 0, 0 };
    blitRegion.dstOffsets[1] = {
        static_cast<int32_t>(m_pSwapChain->getExtent().width),
        static_cast<int32_t>(m_pSwapChain->getExtent().height),
        1
    };

    vkCmdBlitImage(
        commandBuffer,
        m_pLDRImage[imageIndex]->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        m_pSwapChain->getImages()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blitRegion,
        VK_FILTER_NEAREST); // Use VK_FILTER_LINEAR for smoother scaling if needed

    // Transition swapchain image to present mode
    transitionImageLayout(
        commandBuffer,
        m_pSwapChain->getImages()[imageIndex],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,          // From transfer destination
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,               // To present source
        VK_PIPELINE_STAGE_2_TRANSFER_BIT,              // After transfer operations
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,        // Before presentation
        VK_ACCESS_2_TRANSFER_WRITE_BIT,                // After transfer writes
        0,                                             // No access needed for presentation
        VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::cleanup() 
{
    cleanupSwapChain();

    // Clean up the camera
    delete m_pCamera;
    m_pCamera = nullptr;

    for (auto& uniformBuffer : m_pUniformBuffers) 
    {
        delete uniformBuffer;
    }
	for (auto& lightBuffer : m_pLightBuffers) 
    {
		delete lightBuffer;
	}
    for (auto& sunMatricesBuffer : m_pSunMatricesBuffers) 
    {
        delete sunMatricesBuffer;
    }

    vkDestroyImageView(m_pDevice->get(), m_IrradianceMapImageView, nullptr);
    delete m_pIrradianceMapImage;
   
	vkDestroyImageView(m_pDevice->get(), m_SkyboxCubeMapImageView, nullptr);

    delete m_pSkyboxCubeMapImage;

    delete m_pDescriptorManager;
    delete m_pModel;
  
    vmaDestroyAllocator(m_VmaAllocator);

    delete m_pGraphicsPipeline;
	delete m_pDepthPipeline;
	delete m_pShadowMapPipeline;
	delete m_pFinalPipeline;
	delete m_pToneMappingPipeline;
    delete m_pSyncObjects;
    delete m_pCommandPool;
    delete m_pDevice;
    delete m_pSurface;
    delete m_pInstance;
}