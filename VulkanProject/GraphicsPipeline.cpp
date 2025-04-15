// GraphicsPipeline.cpp
#include "GraphicsPipeline.h"

GraphicsPipeline::GraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline)
    : device_(device), pipelineLayout_(pipelineLayout), graphicsPipeline_(graphicsPipeline) {}

GraphicsPipeline::~GraphicsPipeline() {
    if (graphicsPipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    }
}

VkPipelineLayout GraphicsPipeline::getPipelineLayout() const 
{
    return pipelineLayout_;
}

VkPipeline GraphicsPipeline::get() const 
{
    return graphicsPipeline_;
}
