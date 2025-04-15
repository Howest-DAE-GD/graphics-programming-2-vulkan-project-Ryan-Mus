// GraphicsPipeline.h
#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.h>

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline);
    ~GraphicsPipeline();

    VkPipelineLayout getPipelineLayout() const;
    VkPipeline get() const;

private:
    VkDevice device_;
    VkPipelineLayout pipelineLayout_;
    VkPipeline graphicsPipeline_;
};
