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
    : m_pWindow(window) 
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

    createLightBuffer();

	m_Lights.push_back(Light{ glm::vec3(0.0f, 0.2f, 0.0f), glm::vec3(1.0f, 0.3f, 0.3f), 0.5f, 2.0f });
	m_Lights.push_back(Light{ glm::vec3(1.0f, 0.2f, 0.0f), glm::vec3(0.2f, 1.0f, 0.2f), 1.0f, 4.0f });
    m_Lights.push_back(Light{ glm::vec3(2.0f, 0.2f, 0.0f), glm::vec3(0.3f, 0.3f, 1.f), 2.0f, 6.0f });
    m_Lights.push_back(Light{ glm::vec3(3.0f, 0.2f, 0.0f), glm::vec3(1.f, 1.f, 0.2f), 4.0f, 8.0f });

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
		updateLightBuffer(i);
    }

    m_pModel = new Model(m_VmaAllocator, m_pDevice, m_pPhysicalDevice, m_pCommandPool, MODEL_PATH_);
    m_pModel->loadModel();

    size_t materialCount = m_pModel->getMaterials().size();
    m_pDescriptorManager = new DescriptorManager(m_pDevice->get(), MAX_FRAMES_IN_FLIGHT, materialCount);

    // Create descriptor set layouts and pools    
    m_pDescriptorManager->createDescriptorSetLayout();
    m_pDescriptorManager->createFinalPassDescriptorSetLayout();

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
            Texture::getTextureSampler() // Ensure this sampler is created
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


    // Create the final pass graphics pipeline
    m_pFinalPipeline = GraphicsPipelineBuilder()
        .setDevice(m_pDevice->get())
        .setDescriptorSetLayout(m_pDescriptorManager->getFinalPassDescriptorSetLayout()) // Updated
        .setSwapChainExtent(m_pSwapChain->getExtent())
        .setColorFormats({ m_pSwapChain->getImageFormat() })
        .setDepthFormat(VK_FORMAT_UNDEFINED)
        .setVertexInputBindingDescription({})
        .setVertexInputAttributeDescriptions({})
        .setShaderPaths("shaders/final.vert.spv", "shaders/final.frag.spv")
        .setAttachmentCount(1)
        .enableDepthTest(false)
        .setRasterizationState(VK_CULL_MODE_NONE)
        .build();

    m_pSyncObjects = new SynchronizationObjects(m_pDevice->get(), MAX_FRAMES_IN_FLIGHT);
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

   
    // Transition G-buffer images for the current frame
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDiffuseImage->getImage(),
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
        currentGBuffer.pNormalImage->getImage(),
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
        currentGBuffer.pMetallicRougnessImage->getImage(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Transition depth image to DEPTH_STENCIL_ATTACHMENT_OPTIMAL for depth pre-pass
    transitionImageLayout(
        commandBuffer,
        currentGBuffer.pDepthImage->getImage(),
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
                //continue;
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

    // **Memory Barrier for Depth Buffer (Synchronization2)**
    {
        // Ensure depth data is available for the main pass
        VkImageMemoryBarrier2 depthBarrier{};
        depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        depthBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
        depthBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
        depthBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.image = currentGBuffer.pDepthImage->getImage();
        depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthBarrier.subresourceRange.baseMipLevel = 0;
        depthBarrier.subresourceRange.levelCount = 1;
        depthBarrier.subresourceRange.baseArrayLayer = 0;
        depthBarrier.subresourceRange.layerCount = 1;

        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &depthBarrier;

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    }

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
                //continue;
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
        currentGBuffer.pDiffuseImage->getImage(),
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
        currentGBuffer.pNormalImage->getImage(),
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
        currentGBuffer.pMetallicRougnessImage->getImage(),
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
        currentGBuffer.pDepthImage->getImage(),
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        targetDepthLayout,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // **Transition Swapchain Image for Final Pass**
    {
        // Transition swapchain image from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
        transitionImageLayout(
            commandBuffer,
            m_pSwapChain->getImages()[imageIndex],
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_NONE,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
    }

    // **Final Rendering Pass**
    {
        // Begin final rendering pass to the swapchain image
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = m_pSwapChain->getImageViews()[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

        // Set viewport and scissor
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Draw fullscreen triangle (or quad)
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(commandBuffer);
    }

    // Transition swapchain image to PRESENT_SRC_KHR for presentation
    transitionImageLayout(
        commandBuffer,
        m_pSwapChain->getImages()[imageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_NONE,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

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

    // Create or update the camera
    static Camera camera(m_pWindow->getGLFWwindow(),
        glm::vec3(0.0f, 0.0f, 0.3f), // Camera position
        glm::vec3(0.0f, 1.0f, 0.0f), // World up vector (Y-up)
        0.0f, 0.0f);               // Initial yaw and pitch

    camera.update(deltaTime);

    m_UniformBufferObject.model = glm::mat4(1.0f);
    m_UniformBufferObject.view = camera.getViewMatrix();
    m_UniformBufferObject.proj = glm::perspective(
        glm::radians(45.0f),
        m_pSwapChain->getExtent().width / (float)m_pSwapChain->getExtent().height,
        0.001f,
        100.0f);
    m_UniformBufferObject.proj[1][1] *= -1;

    // Set the camera position
    m_UniformBufferObject.cameraPosition = camera.getPosition();

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

    createGBuffer();

    // Update descriptor sets for the final pass with per-frame G-buffers
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
            Texture::getTextureSampler()
        );
    }
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

        m_GBuffers[i].pDiffuseImage->transitionImageLayout(m_pCommandPool,
            m_pDevice->getGraphicsQueue(),
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
          
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

		m_GBuffers[i].pNormalImage->transitionImageLayout(m_pCommandPool,
			m_pDevice->getGraphicsQueue(),
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

		m_GBuffers[i].pMetallicRougnessImage->transitionImageLayout(m_pCommandPool,
			m_pDevice->getGraphicsQueue(),
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

        m_GBuffers[i].pDepthImage->transitionImageLayout(m_pCommandPool,
            m_pDevice->getGraphicsQueue(),
            depthFormat,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
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
    }

    // Clear the vector
    m_GBuffers.clear();

    delete m_pSwapChain;
}


void Renderer::cleanup() 
{
    cleanupSwapChain();


    for (auto& uniformBuffer : m_pUniformBuffers) {
        delete uniformBuffer;
    }
	for (auto& lightBuffer : m_pLightBuffers) {
		delete lightBuffer;
	}

    delete m_pDescriptorManager;
    delete m_pModel;
  
    vmaDestroyAllocator(m_VmaAllocator);

    delete m_pGraphicsPipeline;
	delete m_pDepthPipeline;
	delete m_pFinalPipeline;
    delete m_pSyncObjects;
    delete m_pCommandPool;
    delete m_pDevice;
    delete m_pSurface;
    delete m_pInstance;
}
