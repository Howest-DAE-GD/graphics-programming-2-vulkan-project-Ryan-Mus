#pragma once
#include "ComputePipelineBuilder.h"
#include <stdexcept>

ComputePipelineBuilder& ComputePipelineBuilder::setDevice(Device* device) {
	m_pDevice = device;
	return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setShaderPath(const std::string& shaderFilePath) {
	m_ShaderFilePath = shaderFilePath;
	return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout) {
	m_PipelineLayout = descriptorSetLayout;
	return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setName(const std::string& name)
{
	m_Name = name;
	return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::setPushConstantRange(size_t s)
{
	m_PushConstantSize = s;
	return *this;
}

ComputePipeline* ComputePipelineBuilder::build() 
{
	// Load the compute shader
	std::vector<char> code = readFile(m_ShaderFilePath); // Ensure the shader file is read correctly
	VkShaderModule computeShaderModule = createShaderModule(m_pDevice->get(),code);

	// Create the shader stage info
	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = computeShaderModule;
	shaderStageInfo.pName = "main"; // Entry point in the shader

	//push constant range
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; // Stage this push constant is used in
	pushConstantRange.offset = 0; // Offset in bytes from the start of the push constant block
	pushConstantRange.size = m_PushConstantSize; // Size of the push constant block in bytes

	// Create the pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_PipelineLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1; // Number of push constant ranges
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Pointer to the push constant ranges
	VkPipelineLayout pipelineLayout;

	if (vkCreatePipelineLayout(m_pDevice->get(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	// Create the compute pipeline
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStageInfo;
	pipelineInfo.layout = pipelineLayout;
	VkPipeline computePipeline;

	if (vkCreateComputePipelines(m_pDevice->get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create compute pipeline!");
	}

	// Clean up the shader module
	vkDestroyShaderModule(m_pDevice->get(), computeShaderModule, nullptr);
	return new ComputePipeline(m_pDevice, pipelineLayout, computePipeline);
}