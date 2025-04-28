// GraphicsPipelineBuilder.cpp
#include "GraphicsPipelineBuilder.h"
#include <fstream>
#include <stdexcept>
#include <array>
#include <spdlog/spdlog.h>

namespace 
{
    // Helper function to read shader code from a file
    std::vector<char> readFile(const std::string& filename) 
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) 
        {
            throw std::runtime_error("Failed to open shader file: " + filename);
        }
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();
        return buffer;
    }

    // Helper function to create a shader module
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) 
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();

        // Ensure code size is a multiple of 4
        if (code.size() % 4 != 0) 
        {
            throw std::runtime_error("Shader code size is not a multiple of 4");
        }

        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
        {
            throw std::runtime_error("Failed to create shader module");
        }
		spdlog::debug("Shader module created with code:\"{}\"", code.data());
        return shaderModule;
    }

}  

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDevice(VkDevice device) {
    m_Device = device;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRenderPass(VkRenderPass renderPass) {
    m_RenderPass = renderPass;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout) {
    m_DescriptorSetLayout = descriptorSetLayout;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setSwapChainExtent(VkExtent2D extent) {
    m_SwapChainExtent = extent;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInputBindingDescription(const VkVertexInputBindingDescription& bindingDescription) {
    m_BindingDescription = bindingDescription;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) {
    m_AttributeDescriptions = attributeDescriptions;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setShaderPaths(const std::string& vertShaderPath, const std::string& fragShaderPath) {
    m_VertShaderPath = vertShaderPath;
    m_FragShaderPath = fragShaderPath;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setColorFormats(const std::vector<VkFormat>& colorFormats) {
    m_ColorFormats = colorFormats;
    m_AttachmentCount = static_cast<uint16_t>(colorFormats.size());
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthFormat(VkFormat depthFormat) {
    m_DepthFormat = depthFormat;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setAttachmentCount(uint16_t attachmentCount) {
    m_AttachmentCount = attachmentCount;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enableDepthTest(bool enable) {
	m_DepthTestEnabled = enable;
	return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enableDepthWrite(bool enable) {
	m_DepthWriteEnabled = enable;
	return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setDepthCompareOp(VkCompareOp compareOp)
{
	m_DepthCompareOp = compareOp;
	return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRasterizationState(VkCullModeFlags cullMode)
{
	m_CullMode = cullMode;
	return *this;
}

GraphicsPipeline* GraphicsPipelineBuilder::build() 
{
	spdlog::debug("Building graphics pipeline with vertex shader: {} and fragment shader: {}", m_VertShaderPath, m_FragShaderPath);
    // Load shader code
    auto vertShaderCode = readFile(m_VertShaderPath);
    auto fragShaderCode = readFile(m_FragShaderPath);

    // Create shader modules
    VkShaderModule vertShaderModule = createShaderModule(m_Device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(m_Device, fragShaderCode);

    // Set up shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &m_BindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_AttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = m_AttributeDescriptions.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_SwapChainExtent.width);
    viewport.height = static_cast<float>(m_SwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_SwapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = m_CullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisample state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_DepthTestEnabled;
    depthStencil.depthWriteEnable = m_DepthWriteEnabled;
    depthStencil.depthCompareOp = m_DepthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blend state
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_AttachmentCount);
    VkPipelineColorBlendAttachmentState defaultColorBlendAttachment{};
    defaultColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    defaultColorBlendAttachment.blendEnable = VK_FALSE; // Disable blending

    // Ensure all attachments are identical
    for (auto& attachment : colorBlendAttachments) {
        attachment = defaultColorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    // Dynamic states
    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;      // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;  // Optional

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) 
    {
        vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
        throw std::runtime_error("Failed to create pipeline layout");
    }

    // Pipeline Rendering
    VkPipelineRenderingCreateInfo renderingCreateInfo{};
    renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(m_ColorFormats.size());
    renderingCreateInfo.pColorAttachmentFormats = m_ColorFormats.data();
    renderingCreateInfo.depthAttachmentFormat = m_DepthFormat;
    renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;


    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingCreateInfo; // Attach rendering info
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE; // No render pass
    pipelineInfo.subpass = 0;

	// Create the graphics pipeline
    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) 
    {
        vkDestroyPipelineLayout(m_Device, pipelineLayout, nullptr);
        vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    // Clean up shader modules
    vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);

    return new GraphicsPipeline(m_Device, pipelineLayout, graphicsPipeline);
}
