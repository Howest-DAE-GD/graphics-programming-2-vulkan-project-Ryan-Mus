#pragma once
#include "ComputePipeline.h"

ComputePipeline::ComputePipeline(Device* device,
	VkPipelineLayout pipelineLayout, VkPipeline pipeline)
	: m_pDevice(device), m_PipelineLayout(pipelineLayout), m_Pipeline(pipeline)
{}

ComputePipeline::~ComputePipeline()
{
	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(m_pDevice->get(), m_Pipeline, nullptr);
	}
	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(m_pDevice->get(), m_PipelineLayout, nullptr);
	}
}

VkPipeline ComputePipeline::getPipeline() const
{
	return m_Pipeline;
}

VkPipelineLayout ComputePipeline::getPipelineLayout() const
{
	return m_PipelineLayout;
}