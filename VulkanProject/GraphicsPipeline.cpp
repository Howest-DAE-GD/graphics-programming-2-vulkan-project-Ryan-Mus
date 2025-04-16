#include "GraphicsPipeline.h"
#include <spdlog/spdlog.h>

GraphicsPipeline::GraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline)
    : m_Device(device), m_PipelineLayout(pipelineLayout), m_GraphicsPipeline(graphicsPipeline) 
{
	spdlog::debug("GraphicsPipeline created.");
}

GraphicsPipeline::~GraphicsPipeline() 
{
    if (m_GraphicsPipeline != VK_NULL_HANDLE) 
    {
        vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
    }
    if (m_PipelineLayout != VK_NULL_HANDLE) 
    {
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    }
	spdlog::debug("GraphicsPipeline destroyed.");
}

VkPipelineLayout GraphicsPipeline::getPipelineLayout() const 
{
    return m_PipelineLayout;
}

VkPipeline GraphicsPipeline::get() const 
{
    return m_GraphicsPipeline;
}
