#pragma once
#include <vulkan/vulkan.h>

class RenderPass {
public:
    RenderPass(VkDevice device, VkFormat swapChainImageFormat, VkFormat depthFormat);
    ~RenderPass();

    VkRenderPass get() const;

private:
    void createRenderPass(VkFormat swapChainImageFormat, VkFormat depthFormat);
    VkDevice device_;
    VkRenderPass renderPass_;
};