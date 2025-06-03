#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "Device.h"
#include "ComputePipeline.h"

class ComputePipelineBuilder {
public:
	ComputePipelineBuilder& setDevice(Device* device);
    ComputePipelineBuilder& setShaderPath(const std::string& shaderFilePath);
	ComputePipelineBuilder& setDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
    ComputePipelineBuilder& setName(const std::string& name);
	ComputePipelineBuilder& setPushConstantRange(size_t s);

    ComputePipeline* build();

private:
    Device* m_pDevice;
    VkDescriptorSetLayout m_PipelineLayout{ VK_NULL_HANDLE };
	std::string m_ShaderFilePath;
    std::string m_Name;
	size_t m_PushConstantSize{ 0 };
};
