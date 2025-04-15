// GraphicsPipelineBuilder.h
#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "GraphicsPipeline.h"

class GraphicsPipelineBuilder {
public:
    GraphicsPipelineBuilder& setDevice(VkDevice device);
    GraphicsPipelineBuilder& setRenderPass(VkRenderPass renderPass);
    GraphicsPipelineBuilder& setDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
    GraphicsPipelineBuilder& setSwapChainExtent(VkExtent2D extent);
    GraphicsPipelineBuilder& setVertexInputBindingDescription(const VkVertexInputBindingDescription& bindingDescription);
    GraphicsPipelineBuilder& setVertexInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
    GraphicsPipelineBuilder& setShaderPaths(const std::string& vertShaderPath, const std::string& fragShaderPath);
    GraphicsPipeline* build();

private:
    VkDevice device_{ VK_NULL_HANDLE };
    VkRenderPass renderPass_{ VK_NULL_HANDLE };
    VkDescriptorSetLayout descriptorSetLayout_{ VK_NULL_HANDLE };
    VkExtent2D swapChainExtent_{};
    VkVertexInputBindingDescription bindingDescription_{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions_{};
    std::string vertShaderPath_;
    std::string fragShaderPath_;
};
