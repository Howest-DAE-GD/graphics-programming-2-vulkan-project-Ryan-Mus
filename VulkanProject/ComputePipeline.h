#pragma once

#include <vulkan/vulkan.h>
#include "Device.h"

class ComputePipeline {
public:
    ComputePipeline(Device* device, VkPipelineLayout pipelineLayout, VkPipeline pipeline);
    ~ComputePipeline();

    VkPipeline getPipeline() const;
    VkPipelineLayout getPipelineLayout() const;

private:
    Device* m_pDevice;
    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_Pipeline;
};
