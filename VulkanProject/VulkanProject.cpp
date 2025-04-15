#include "Window.h"
#include "Instance.h"
#include "InstanceBuilder.h"
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
#include "Buffer.h"
#include "DescriptorManager.h"
#include "Image.h"
#include "Model.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <spdlog/spdlog.h>

#include <iostream>
#include <optional>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <limits> 
#include <fstream>
#include <filesystem>
#include <array>
#include <functional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

const int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject 
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<const char*> validationLayers = 
{
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = 
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class HelloTriangleApplication 
{
public:
    void run()
    {
        window = new Window(WIDTH, HEIGHT, "Vulkan");
        initVulkan();
        mainLoop();
        cleanup();
    }
private:
    void initVulkan() 
    {
		instance = new Instance();

		surface = new Surface(instance->getInstance(), window->getGLFWwindow());

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        physicalDevice = PhysicalDeviceBuilder()
            .setInstance(instance->getInstance())
            .setSurface(surface->get())
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

        auto localDevice = DeviceBuilder()
            .setPhysicalDevice(physicalDevice->get())
            .setQueueFamilyIndices(physicalDevice->getQueueFamilyIndices())
            .addRequiredExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
            .setEnabledFeatures(physicalDevice->getFeatures())
            .enableValidationLayers(validationLayers)
            .build();
        device = std::move(localDevice);

        swapChain = SwapChainBuilder()
            .setDevice(device->get())
            .setPhysicalDevice(physicalDevice->get())
            .setSurface(surface->get())
            .setWidth(WIDTH)
            .setHeight(HEIGHT)
            .setGraphicsFamilyIndex(physicalDevice->getQueueFamilyIndices().graphicsFamily.value())
            .setPresentFamilyIndex(physicalDevice->getQueueFamilyIndices().presentFamily.value())
            .build();

        renderPass = new RenderPass(device->get(), swapChain->getImageFormat(),findDepthFormat());
 
        descriptorManager = new DescriptorManager(device->get(), MAX_FRAMES_IN_FLIGHT);
		descriptorManager->createDescriptorSetLayout();

        auto localGraphicsPipeline = GraphicsPipelineBuilder()
            .setDevice(device->get())
            .setRenderPass(renderPass->get())
            .setDescriptorSetLayout(descriptorManager->getDescriptorSetLayout())
            .setSwapChainExtent(swapChain->getExtent())
            .setVertexInputBindingDescription(Vertex::getBindingDescription())
            .setVertexInputAttributeDescriptions(Vertex::getAttributeDescriptions())
            .setShaderPaths("shaders/vert.spv", "shaders/frag.spv")
            .build();

		graphicsPipeline = std::move(localGraphicsPipeline);

        commandPool = new CommandPool{ device->get(),
            physicalDevice->getQueueFamilyIndices().graphicsFamily.value() };

        createVmaAllocator();
        createDepthResources();
        createFramebuffers();

        createTextureImage();
        createTextureImageView();
        createTextureSampler();

        model = new Model(vmaAllocator, device, commandPool, MODEL_PATH);
        model->loadModel();
        model->createVertexBuffer();
        model->createIndexBuffer();
        createUniformBuffers();

        // Gather uniform buffer handles
        std::vector<VkBuffer> uniformBufferHandles;
        for (const auto& buffer : uniformBuffers) {
            uniformBufferHandles.push_back(buffer->get());
        }

        // Create descriptor sets
        descriptorManager->createDescriptorSets(
            uniformBufferHandles, textureImageView, textureSampler, sizeof(UniformBufferObject));

        createCommandBuffers();
        //createSyncObjects();
        syncObjects = new SynchronizationObjects(device->get(), MAX_FRAMES_IN_FLIGHT);
  
    }
    
    bool hasStencilComponent(VkFormat format) 
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat findDepthFormat() 
    {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
    {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice->get(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    void createDepthResources() 
    {
        VkFormat depthFormat = findDepthFormat();
        depthImage = new Image(device->get(), vmaAllocator);
        depthImage->createImage(swapChain->getExtent().width, swapChain->getExtent().height,
            depthFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        depthImageView = depthImage->createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        depthImage->transitionImageLayout(commandPool->get(), device->getGraphicsQueue(),
            depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    void createTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice->get(), &properties);

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(device->get(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture sampler!");
		}
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device->get(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void createTextureImageView() 
    {
        textureImageView = textureImage->createImageView(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = commandPool->beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        commandPool->endSingleTimeCommands(commandBuffer,device->getGraphicsQueue());
    }

    void createTextureImage() 
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        Buffer stagingBuffer(
            vmaAllocator,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );

        void* data = stagingBuffer.map();
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        stagingBuffer.unmap();

        stbi_image_free(pixels);

        textureImage = new Image(device->get(), vmaAllocator);
        textureImage->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        textureImage->transitionImageLayout(commandPool->get(), device->getGraphicsQueue(),
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage(stagingBuffer.get(), textureImage->getImage(), texWidth, texHeight);

        textureImage->transitionImageLayout(commandPool->get(), device->getGraphicsQueue(),
            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        //vmaDestroyBuffer(vmaAllocator, stagingBuffer, stagingBufferAllocation);
    }

    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            uniformBuffers[i] = new Buffer(
                vmaAllocator,
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT
            );
        }
    }

    void createVmaAllocator()
    {
        VmaAllocatorCreateInfo createInfo = {};
        createInfo.physicalDevice = physicalDevice->get();
        createInfo.device = device->get();
        createInfo.instance = instance->getInstance();
        createInfo.flags = 0;
        vmaCreateAllocator(&createInfo, &vmaAllocator);
    }

    void cleanupSwapChain() 
    {
        vkDestroyImageView(device->get(), depthImageView, nullptr);
        delete depthImage;


        for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
        {
            vkDestroyFramebuffer(device->get(), swapChainFramebuffers[i], nullptr);
        }

        // **Properly reset the swapChain instead of calling the destructor directly**
        swapChain.reset();
    }

    void recreateSwapChain()
    {
        int width = 0, height = 0;
        window->getFramebufferSize(width, height);
        while (width == 0 || height == 0)
        {
            window->getFramebufferSize(width, height);
            window->pollEvents();
        }

        vkDeviceWaitIdle(device->get());

        cleanupSwapChain();

        swapChain = SwapChainBuilder()
            .setDevice(device->get())
            .setPhysicalDevice(physicalDevice->get())
            .setSurface(surface->get())
            .setWidth(width)
            .setHeight(height)
            .setGraphicsFamilyIndex(physicalDevice->getQueueFamilyIndices().graphicsFamily.value())
            .setPresentFamilyIndex(physicalDevice->getQueueFamilyIndices().presentFamily.value())
            .build();

        createDepthResources();
        createFramebuffers();
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        //cmds
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass->get();
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChain->getExtent();
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->get());

            // Bind vertex and index buffers from the model
            VkBuffer vertexBuffers[] = { model->getVertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
           
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, model->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChain->getExtent().width);
            viewport.height = static_cast<float>(swapChain->getExtent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChain->getExtent();
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline->getPipelineLayout(),
                0,
                1,
                &descriptorManager->getDescriptorSets()[currentFrame],
                0,
                nullptr); 
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model->getIndexCount()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to record command buffer!");
        }

    }

    void createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool->get();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device->get(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void createFramebuffers() 
    {
        swapChainFramebuffers.resize(swapChain->getImageViews().size());

        for (size_t i = 0; i < swapChain->getImageViews().size(); i++)
        {
            std::array<VkImageView, 2> attachments = 
            {
                swapChain->getImageViews()[i],
                depthImageView
            };
            // Log the image views being used for the framebuffer
            spdlog::info("Creating framebuffer with image view {}: {}", i, static_cast<void*>(attachments[0]));
            spdlog::info("Creating framebuffer with depth image view: {}", static_cast<void*>(attachments[1]));


            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass->get();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChain->getExtent().width;
            framebufferInfo.height = swapChain->getExtent().height;
            framebufferInfo.layers = 1;
            framebufferInfo.flags = 0;

			if (framebufferInfo.pAttachments == nullptr)
			{
				spdlog::error("Framebuffer attachments are null!");
			}
            
            if (vkCreateFramebuffer(device->get(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void mainLoop()
    {
        while (!window->shouldClose())
        {
            window->pollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(device->get());
    }

    void cleanup()
    {
        cleanupSwapChain();

		for (auto& uniformBuffer : uniformBuffers)
		{
			delete uniformBuffer;
		}

        delete descriptorManager;

		delete model;

        vkDestroySampler(device->get(), textureSampler, nullptr);

        vkDestroyImageView(device->get(), textureImageView, nullptr);

		delete textureImage;

        vmaDestroyAllocator(vmaAllocator);

		delete graphicsPipeline;

        delete renderPass;

		delete syncObjects;

		delete commandPool;

		delete device;

        delete surface;

        delete instance;

        delete window;
    }
    
    void drawFrame() 
    {
        vkWaitForFences(device->get(), 1, syncObjects->getInFlightFence(currentFrame), VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device->get(), swapChain->get(), UINT64_MAX, *syncObjects->getImageAvailableSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) 
        {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // Only reset the fence if we are submitting work
        vkResetFences(device->get(), 1, syncObjects->getInFlightFence(currentFrame));

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        updateUniformBuffer(currentFrame);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { *syncObjects->getImageAvailableSemaphore(currentFrame) };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { *syncObjects->getRenderFinishedSemaphore(currentFrame) };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, *syncObjects->getInFlightFence(currentFrame)) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain->get()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->isFramebufferResized())
        {
            window->resetFramebufferResized();
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void updateUniformBuffer(uint32_t currentImage) 
    {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChain->getExtent().width / (float)swapChain->getExtent().height, 0.1f, 10.0f);

        ubo.proj[1][1] *= -1;

        void* data = uniformBuffers[currentImage]->map();
        memcpy(data, &ubo, sizeof(ubo));
    }

    Window* window;

	//Vulkan
    Instance* instance;

    Surface* surface;

    std::optional<PhysicalDevice> physicalDevice;
    Device* device;

    std::optional<SwapChain> swapChain;

	DescriptorManager* descriptorManager;

    RenderPass* renderPass;

    GraphicsPipeline* graphicsPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    CommandPool* commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    SynchronizationObjects* syncObjects;

    uint32_t currentFrame = 0;

    bool framebufferResized = false;

	Model* model;

    std::vector<Buffer*> uniformBuffers;

	VmaAllocator vmaAllocator = nullptr;

    Image* textureImage;
    VkImageView textureImageView;
    VkSampler textureSampler;

    Image* depthImage;
    VkImageView depthImageView;
};

int main() 
{
    HelloTriangleApplication app;

    try 
    {
        app.run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}