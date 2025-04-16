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
    : window_(window) {
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::initialize() {
    initVulkan();
}

void Renderer::initVulkan() {
    instance_ = new Instance();

    surface_ = new Surface(instance_->getInstance(), window_->getGLFWwindow());

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    physicalDevice_ = PhysicalDeviceBuilder()
        .setInstance(instance_->getInstance())
        .setSurface(surface_->get())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .setRequiredDeviceFeatures(deviceFeatures)
        .build();

    std::vector<const char*> validationLayers;
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    if (enableValidationLayers) {
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    device_ = DeviceBuilder()
        .setPhysicalDevice(physicalDevice_->get())
        .setQueueFamilyIndices(physicalDevice_->getQueueFamilyIndices())
        .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
        .setEnabledFeatures(physicalDevice_->getFeatures())
        .enableValidationLayers(validationLayers)
        .build();

    swapChain_ = SwapChainBuilder()
        .setDevice(device_->get())
        .setPhysicalDevice(physicalDevice_->get())
        .setSurface(surface_->get())
        .setWidth(window_->getWidth())
        .setHeight(window_->getHeight())
        .setGraphicsFamilyIndex(physicalDevice_->getQueueFamilyIndices().graphicsFamily.value())
        .setPresentFamilyIndex(physicalDevice_->getQueueFamilyIndices().presentFamily.value())
        .build();

    renderPass_ = new RenderPass(device_->get(), swapChain_->getImageFormat(), findDepthFormat());

    descriptorManager_ = new DescriptorManager(device_->get(), MAX_FRAMES_IN_FLIGHT);
    descriptorManager_->createDescriptorSetLayout();

    graphicsPipeline_ = GraphicsPipelineBuilder()
        .setDevice(device_->get())
        .setRenderPass(renderPass_->get())
        .setDescriptorSetLayout(descriptorManager_->getDescriptorSetLayout())
        .setSwapChainExtent(swapChain_->getExtent())
        .setVertexInputBindingDescription(Vertex::getBindingDescription())
        .setVertexInputAttributeDescriptions(Vertex::getAttributeDescriptions())
        .setShaderPaths("shaders/vert.spv", "shaders/frag.spv")
        .build();

    commandPool_ = new CommandPool(device_->get(), physicalDevice_->getQueueFamilyIndices().graphicsFamily.value());

    createVmaAllocator();
    createDepthResources();
    createFramebuffers();

    texture_ = new Texture(device_, vmaAllocator_, commandPool_, TEXTURE_PATH_, physicalDevice_->get());

    std::vector<Texture*> textures(MAX_FRAMES_IN_FLIGHT, texture_);

    model_ = new Model(vmaAllocator_, device_, commandPool_, MODEL_PATH_);
    model_->loadModel();
    model_->createVertexBuffer();
    model_->createIndexBuffer();

    createUniformBuffers();

    // Gather uniform buffer handles
    std::vector<VkBuffer> uniformBufferHandles;
    for (const auto& buffer : uniformBuffers_) {
        uniformBufferHandles.push_back(buffer->get());
    }

    // Create descriptor sets
    descriptorManager_->createDescriptorSets(
        uniformBufferHandles, textures, sizeof(UniformBufferObject));

    createCommandBuffers();

    syncObjects_ = new SynchronizationObjects(device_->get(), MAX_FRAMES_IN_FLIGHT);
}

void Renderer::createVmaAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice_->get();
    allocatorInfo.device = device_->get();
    allocatorInfo.instance = instance_->getInstance();
    vmaCreateAllocator(&allocatorInfo, &vmaAllocator_);
}

void Renderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    depthImage_ = new Image(device_, vmaAllocator_);
    depthImage_->createImage(
        swapChain_->getExtent().width,
        swapChain_->getExtent().height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    depthImageView_ = depthImage_->createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    depthImage_->transitionImageLayout(
        commandPool_->get(),
        device_->getGraphicsQueue(),
        depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat Renderer::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Renderer::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    VkImageTiling tiling,
    VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice_->get(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

void Renderer::createFramebuffers() {
    swapChainFramebuffers_.resize(swapChain_->getImageViews().size());

    for (size_t i = 0; i < swapChain_->getImageViews().size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChain_->getImageViews()[i],
            depthImageView_
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_->get();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChain_->getExtent().width;
        framebufferInfo.height = swapChain_->getExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device_->get(), &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Renderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers_[i] = new Buffer(
            vmaAllocator_,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
    }
}

void Renderer::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_->get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device_->get(), &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_->get();
    renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain_->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_->get());

        // Bind vertex and index buffers from the model
        VkBuffer vertexBuffers[] = { model_->getVertexBuffer() };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, model_->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChain_->getExtent().width);
        viewport.height = static_cast<float>(swapChain_->getExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChain_->getExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline_->getPipelineLayout(),
            0,
            1,
            &descriptorManager_->getDescriptorSets()[currentFrame_],
            0,
            nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model_->getIndexCount()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Renderer::drawFrame() {
    vkWaitForFences(device_->get(), 1, syncObjects_->getInFlightFence(currentFrame_), VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device_->get(),
        swapChain_->get(),
        UINT64_MAX,
        *syncObjects_->getImageAvailableSemaphore(currentFrame_),
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(device_->get(), 1, syncObjects_->getInFlightFence(currentFrame_));

    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    updateUniformBuffer(currentFrame_);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { *syncObjects_->getImageAvailableSemaphore(currentFrame_) };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];

    VkSemaphore signalSemaphores[] = { *syncObjects_->getRenderFinishedSemaphore(currentFrame_) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device_->getGraphicsQueue(), 1, &submitInfo, *syncObjects_->getInFlightFence(currentFrame_)) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain_->get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(device_->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_->isFramebufferResized()) {
        window_->resetFramebufferResized();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        swapChain_->getExtent().width / (float)swapChain_->getExtent().height,
        0.1f,
        10.0f);
    ubo.proj[1][1] *= -1;

    void* data = uniformBuffers_[currentImage]->map();
    memcpy(data, &ubo, sizeof(ubo));
}

void Renderer::recreateSwapChain() {
    int width = 0, height = 0;
    window_->getFramebufferSize(width, height);
    while (width == 0 || height == 0) {
        window_->getFramebufferSize(width, height);
        window_->pollEvents();
    }

    vkDeviceWaitIdle(device_->get());

    cleanupSwapChain();

    swapChain_ = SwapChainBuilder()
        .setDevice(device_->get())
        .setPhysicalDevice(physicalDevice_->get())
        .setSurface(surface_->get())
        .setWidth(width)
        .setHeight(height)
        .setGraphicsFamilyIndex(physicalDevice_->getQueueFamilyIndices().graphicsFamily.value())
        .setPresentFamilyIndex(physicalDevice_->getQueueFamilyIndices().presentFamily.value())
        .build();

    createDepthResources();
    createFramebuffers();
}

void Renderer::cleanupSwapChain() {
    vkDestroyImageView(device_->get(), depthImageView_, nullptr);
    delete depthImage_;

    for (auto framebuffer : swapChainFramebuffers_) {
        vkDestroyFramebuffer(device_->get(), framebuffer, nullptr);
    }

    delete swapChain_;
}

void Renderer::cleanup() {
    cleanupSwapChain();

    for (auto& uniformBuffer : uniformBuffers_) {
        delete uniformBuffer;
    }

    delete descriptorManager_;
    delete model_;
    delete texture_;
    vmaDestroyAllocator(vmaAllocator_);

    delete graphicsPipeline_;
    delete renderPass_;
    delete syncObjects_;
    delete commandPool_;
    delete device_;
    delete surface_;
    delete instance_;
}
