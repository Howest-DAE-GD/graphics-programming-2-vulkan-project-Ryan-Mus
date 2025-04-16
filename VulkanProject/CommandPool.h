#pragma once
#include <vulkan/vulkan.h>

class CommandPool 
{
public:
    CommandPool(VkDevice device, uint32_t queueFamilyIndex);
    ~CommandPool();

    VkCommandPool get() const;

    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void freeCommandBuffer(VkCommandBuffer commandBuffer);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

private:
    VkDevice m_Device;
    VkCommandPool m_CommandPool;
};
