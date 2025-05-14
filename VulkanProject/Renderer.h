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
#include "ComputePipelineBuilder.h"
#include "SynchronizationObjects.h"
#include "CommandPool.h"
#include "DescriptorManager.h"
#include "Model.h"
#include "Texture.h"
#include "Buffer.h"
#include "Image.h"
#include "Camera.h"
#include "vk_mem_alloc.h"

#include <vector>
#include <string>
#include <array>



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
    void createGBuffer();
	void createHDRImage();
	void createLDRImage();
    void createUniformBuffers();
	void createLightBuffer();
    void createCommandBuffers();
    void createSkyboxCubeMap();
	void createIrradianceMap();
    void renderShadowMap();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateUniformBuffer(uint32_t currentImage);
	void updateLightBuffer(uint32_t currentImage);
    void recreateSwapChain();
    void cleanupSwapChain();
	void blitLDRToSwapchain(uint32_t imageIndex, VkCommandBuffer commandBuffer);

    // Helper functions
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    
    void renderToCubeMap(
        Image* pInputImage,
        VkImageView inputImageView,
        Image* pOutputCubeMapImage,
        std::array<VkImageView, 6> outputCubeMapImageViews,
        VkSampler sampler,
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );

	//Pure Vulkan function
    void transitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask,
        VkPipelineStageFlags2 dstStageMask,
        VkAccessFlags2 srcAccessMask,
        VkAccessFlags2 dstAccessMask,
		VkImageAspectFlags aspectMask);

	// Uses Image class
    void transitionImageLayout(
        VkCommandBuffer commandBuffer,
        Image* pImage,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask,
        VkPipelineStageFlags2 dstStageMask,
        VkAccessFlags2 srcAccessMask,
        VkAccessFlags2 dstAccessMask,
        VkImageAspectFlags aspectMask);

    void transitionSwapchainImagesToPresentLayout();

    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 cameraPosition;
		alignas(16) glm::vec2 viewportSize;
    };

    struct GBuffer
    {
        Image* pDiffuseImage;
		VkImageView diffuseImageView;

		Image* pNormalImage;
        VkImageView normalImageView;

        Image* pMetallicRougnessImage;
        VkImageView metallicRoughnessImageView;

        Image* pDepthImage;
		VkImageView depthImageView;

		Image* pShadowMapImage;
		VkImageView shadowMapImageView;
    };

	struct Light
	{
		alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 color;
		alignas(4) float intensity;
        alignas(4) float radius;
	};

    Window* m_pWindow;

    // Vulkan components
    Instance* m_pInstance;
    Surface* m_pSurface;
    PhysicalDevice* m_pPhysicalDevice;
    Device* m_pDevice;
    SwapChain* m_pSwapChain;
    DescriptorManager* m_pDescriptorManager;
    GraphicsPipeline* m_pGraphicsPipeline;
	GraphicsPipeline* m_pDepthPipeline;
	GraphicsPipeline* m_pFinalPipeline;
	GraphicsPipeline* m_pShadowMapPipeline;
	ComputePipeline* m_pToneMappingPipeline;
    CommandPool* m_pCommandPool;
    SynchronizationObjects* m_pSyncObjects;

    // Resources
    Model* m_pModel;
    std::vector<Buffer*> m_pUniformBuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VmaAllocator m_VmaAllocator = nullptr;

    uint32_t m_currentFrame = 0;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    static constexpr int MAX_LIGHT_COUNT = 10;

	// modelprojview matrix + camera position + viewport size
    UniformBufferObject m_UniformBufferObject{};
	//Light pass buffer
    std::vector<GBuffer> m_GBuffers;
	std::vector<Light> m_Lights;
	std::vector<Buffer*> m_pLightBuffers;

	// HDR and LDR images
	std::vector<Image*> m_pHDRImage;
	std::vector<VkImageView> m_HDRImageView;

	std::vector<Image*> m_pLDRImage;
	std::vector<VkImageView> m_LDRImageView;

	//HDRI -> Cube map -> Irradiance map
    Image* m_pSkyboxCubeMapImage;
    std::array<VkImageView,6> m_SkyboxCubeMapImageViews;
	VkImageView m_SkyboxCubeMapImageView;

	Image* m_pIrradianceMapImage;
	std::array<VkImageView, 6> m_IrradianceMapImageViews;
	VkImageView m_IrradianceMapImageView;

    // Paths
    const std::string MODEL_PATH_ = "models/glTF/Sponza.gltf";
	const std::string HDRI_PATH_ = "default/circus_arena_2k.hdr";
};
