#pragma once

#include "Window.h"
#include "Instance.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "PhysicalDeviceBuilder.h"
#include "Device.h"
#include "DeviceBuilder.h"
#include "SwapChain.h"
#include "SwapChainBuilder.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "GraphicsPipelineBuilder.h"
#include "SynchronizationObjects.h"
#include "CommandPool.h"
#include "DescriptorManager.h"
#include "Model.h"
#include "Texture.h"
#include "Buffer.h"
#include "Image.h"
#include "vk_mem_alloc.h"

#include <vector>
#include <string>

class Renderer {
public:
    Renderer(Window* window);
    ~Renderer();

    void initialize();
    void drawFrame();
    void cleanup();

	VkDevice getDevice() const { return device_->get(); }

private:
    void initVulkan();
    void createVmaAllocator();
    void createDepthResources();
    void createFramebuffers();
    void createUniformBuffers();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateUniformBuffer(uint32_t currentImage);
    void recreateSwapChain();
    void cleanupSwapChain();

    // Helper functions
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    Window* window_;

    // Vulkan components
    Instance* instance_;
    Surface* surface_;
    PhysicalDevice* physicalDevice_;
    Device* device_;
    SwapChain* swapChain_;
    RenderPass* renderPass_;
    DescriptorManager* descriptorManager_;
    GraphicsPipeline* graphicsPipeline_;
    CommandPool* commandPool_;
    SynchronizationObjects* syncObjects_;

    // Resources
    Texture* texture_;
    Model* model_;
    std::vector<Buffer*> uniformBuffers_;
    std::vector<VkCommandBuffer> commandBuffers_;
    std::vector<VkFramebuffer> swapChainFramebuffers_;
    Image* depthImage_;
    VkImageView depthImageView_;
    VmaAllocator vmaAllocator_ = nullptr;

    uint32_t currentFrame_ = 0;

    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Paths
    const std::string MODEL_PATH_ = "models/viking_room.obj";
    const std::string TEXTURE_PATH_ = "textures/viking_room.png";
};
