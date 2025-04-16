// GraphicsPipeline.h
#pragma once
#include <vulkan/vulkan.h>

class GraphicsPipeline 
{
public:
    GraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline);
    ~GraphicsPipeline();

    VkPipelineLayout getPipelineLayout() const;
    VkPipeline get() const;

private:
    VkDevice m_Device;
    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_GraphicsPipeline;
};
