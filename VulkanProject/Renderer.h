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

class Renderer 
{
public:
    Renderer(Window* window);
    ~Renderer();

    void initialize();
    void drawFrame();
    void cleanup();

	VkDevice getDevice() const { return m_pDevice->get(); }

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

    Window* m_pWindow;

    // Vulkan components
    Instance* m_pInstance;
    Surface* m_pSurface;
    PhysicalDevice* m_pPhysicalDevice;
    Device* m_pDevice;
    SwapChain* m_pSwapChain;
    RenderPass* m_pRenderPass;
    DescriptorManager* m_pDescriptorManager;
    GraphicsPipeline* m_pGraphicsPipeline;
    CommandPool* m_pCommandPool;
    SynchronizationObjects* m_pSyncObjects;

    // Resources
    Texture* m_pTexture;
    Model* m_pModel;
    std::vector<Buffer*> m_pUniformBuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    Image* m_pDepthImage;
    VkImageView m_DepthImageView;
    VmaAllocator m_VmaAllocator = nullptr;

    uint32_t m_currentFrame = 0;

    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Paths
    const std::string MODEL_PATH_ = "models/viking_room.obj";
    const std::string TEXTURE_PATH_ = "textures/viking_room.png";
};
