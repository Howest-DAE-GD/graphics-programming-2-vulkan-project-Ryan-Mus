#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "GraphicsPipeline.h"

class GraphicsPipelineBuilder
{
public:
    GraphicsPipelineBuilder& setDevice(VkDevice device);
    GraphicsPipelineBuilder& setRenderPass(VkRenderPass renderPass);
    GraphicsPipelineBuilder& setDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
    GraphicsPipelineBuilder& setSwapChainExtent(VkExtent2D extent);
    GraphicsPipelineBuilder& setVertexInputBindingDescription(const VkVertexInputBindingDescription& bindingDescription);
    GraphicsPipelineBuilder& setVertexInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
    GraphicsPipelineBuilder& setShaderPaths(const std::string& vertShaderPath, const std::string& fragShaderPath);
    GraphicsPipelineBuilder& setColorFormats(const std::vector<VkFormat>& colorFormats); // Updated to support multiple formats
    GraphicsPipelineBuilder& setDepthFormat(VkFormat depthFormat);
    GraphicsPipelineBuilder& setAttachmentCount(uint16_t attachmentCount);

    GraphicsPipeline* build();

private:
    VkDevice m_Device{ VK_NULL_HANDLE };
    VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
    VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
    VkExtent2D m_SwapChainExtent{};
    VkVertexInputBindingDescription m_BindingDescription{};
    std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions{};
    std::string m_VertShaderPath;
    std::string m_FragShaderPath;
    std::vector<VkFormat> m_ColorFormats{}; // Store multiple color formats
    VkFormat m_DepthFormat{ VK_FORMAT_UNDEFINED };
    uint16_t m_AttachmentCount{ 0 };
};

