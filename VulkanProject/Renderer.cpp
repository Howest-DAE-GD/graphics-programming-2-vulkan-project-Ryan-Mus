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

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

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

    m_pPhysicalDevice = PhysicalDeviceBuilder()
        .setInstance(m_pInstance->getInstance())
        .setSurface(m_pSurface->get())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .addRequiredExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
        .addRequiredExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
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
        .build();

    m_pRenderPass = new RenderPass(m_pDevice->get(), m_pSwapChain->getImageFormat(), findDepthFormat());

    m_pDescriptorManager = new DescriptorManager(m_pDevice->get(), MAX_FRAMES_IN_FLIGHT,0);
    m_pDescriptorManager->createDescriptorSetLayout();

    m_pCommandPool = new CommandPool(m_pDevice->get(), m_pPhysicalDevice->getQueueFamilyIndices().graphicsFamily.value());

    createVmaAllocator();
    createDepthResources();
    createFramebuffers();

    m_pModel = new Model(m_VmaAllocator, m_pDevice,m_pPhysicalDevice, m_pCommandPool, MODEL_PATH_);
    m_pModel->loadModel();
    m_pModel->createVertexBuffer();
    m_pModel->createIndexBuffer();

    createUniformBuffers();

    // Gather uniform buffer handles
    std::vector<VkBuffer> uniformBufferHandles;
    for (const auto& buffer : m_pUniformBuffers) {
        uniformBufferHandles.push_back(buffer->get());
    }

    // Get textures from the Model
    std::vector<Texture*> textures = m_pModel->getTextures();

	m_pDescriptorManager->SetTextureCount(textures.size());
    // Collect image infos for the textures
    std::vector<VkDescriptorImageInfo> imageInfos;
 
    // Create descriptor sets
    m_pDescriptorManager->createDescriptorSets(
        uniformBufferHandles, textures, sizeof(UniformBufferObject));

    createCommandBuffers();

    m_pGraphicsPipeline = GraphicsPipelineBuilder()
        .setDevice(m_pDevice->get())
        .setRenderPass(m_pRenderPass->get())
        .setDescriptorSetLayout(m_pDescriptorManager->getDescriptorSetLayout())
        .setSwapChainExtent(m_pSwapChain->getExtent())
        .setVertexInputBindingDescription(Vertex::getBindingDescription())
        .setVertexInputAttributeDescriptions(Vertex::getAttributeDescriptions())
        .setShaderPaths("shaders/vert.spv", "shaders/frag.spv")
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

void Renderer::createDepthResources() 
{
    VkFormat depthFormat = findDepthFormat();
    m_pDepthImage = new Image(m_pDevice, m_VmaAllocator);
    m_pDepthImage->createImage(
        m_pSwapChain->getExtent().width,
        m_pSwapChain->getExtent().height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    m_DepthImageView = m_pDepthImage->createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    m_pDepthImage->transitionImageLayout(
        m_pCommandPool->get(),
        m_pDevice->getGraphicsQueue(),
        depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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

void Renderer::createFramebuffers() 
{
    m_SwapChainFramebuffers.resize(m_pSwapChain->getImageViews().size());

    for (size_t i = 0; i < m_pSwapChain->getImageViews().size(); i++) {
        std::array<VkImageView, 2> attachments = {
            m_pSwapChain->getImageViews()[i],
            m_DepthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_pRenderPass->get();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_pSwapChain->getExtent().width;
        framebufferInfo.height = m_pSwapChain->getExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_pDevice->get(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
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
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_pRenderPass->get();
    renderPassInfo.framebuffer = m_SwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_pSwapChain->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGraphicsPipeline->get());

        // Bind vertex and index buffers from the model
        VkBuffer vertexBuffers[] = { m_pModel->getVertexBuffer() };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_pModel->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_pSwapChain->getExtent().width);
        viewport.height = static_cast<float>(m_pSwapChain->getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_pSwapChain->getExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pGraphicsPipeline->getPipelineLayout(),
            0,
            1,
            &m_pDescriptorManager->getDescriptorSets()[m_currentFrame],
            0,
            nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_pModel->getIndexCount()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
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
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_pDevice->get(), 1, m_pSyncObjects->getInFlightFence(m_currentFrame));

    vkResetCommandBuffer(m_CommandBuffers[m_currentFrame], 0);
    recordCommandBuffer(m_CommandBuffers[m_currentFrame], imageIndex);

    updateUniformBuffer(m_currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { *m_pSyncObjects->getImageAvailableSemaphore(m_currentFrame) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[m_currentFrame];

    VkSemaphore signalSemaphores[] = { *m_pSyncObjects->getRenderFinishedSemaphore(m_currentFrame) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_pDevice->getGraphicsQueue(), 1, &submitInfo, *m_pSyncObjects->getInFlightFence(m_currentFrame)) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_pSwapChain->get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_pDevice->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_pWindow->isFramebufferResized()) {
        m_pWindow->resetFramebufferResized();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
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
        glm::vec3(0.0f, 0.0f, 3.0f), // Camera position
        glm::vec3(0.0f, 1.0f, 0.0f), // World up vector (Z-up)
        0.0f, 0.0f);               // Initial yaw and pitch


    camera.update(deltaTime);

    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera.getViewMatrix();
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        m_pSwapChain->getExtent().width / (float)m_pSwapChain->getExtent().height,
        0.1f,
        10000.0f);
    ubo.proj[1][1] *= -1;

    void* data = m_pUniformBuffers[currentImage]->map();
    memcpy(data, &ubo, sizeof(ubo));
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
        .build();

    createDepthResources();
    createFramebuffers();
}

void Renderer::cleanupSwapChain() 
{
    vkDestroyImageView(m_pDevice->get(), m_DepthImageView, nullptr);
    delete m_pDepthImage;

    for (auto framebuffer : m_SwapChainFramebuffers) {
        vkDestroyFramebuffer(m_pDevice->get(), framebuffer, nullptr);
    }

    delete m_pSwapChain;
}

void Renderer::cleanup() 
{
    cleanupSwapChain();

    for (auto& uniformBuffer : m_pUniformBuffers) {
        delete uniformBuffer;
    }

    delete m_pDescriptorManager;
    delete m_pModel;
    //delete m_pTexture;
    vmaDestroyAllocator(m_VmaAllocator);

    delete m_pGraphicsPipeline;
    delete m_pRenderPass;
    delete m_pSyncObjects;
    delete m_pCommandPool;
    delete m_pDevice;
    delete m_pSurface;
    delete m_pInstance;
}
