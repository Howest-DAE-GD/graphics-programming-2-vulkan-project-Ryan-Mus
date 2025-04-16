#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <spdlog/spdlog.h>

class SynchronizationObjects 
{
public:
    SynchronizationObjects(VkDevice device, size_t maxFramesInFlight)
        : m_Device(device), m_MaxFramesInFlight(maxFramesInFlight) 
    {
        m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
        m_InFlightFences.resize(m_MaxFramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < m_MaxFramesInFlight; i++) 
        {
            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
		spdlog::debug("Semaphores: {} image available, {} render finished,\n Fences {} in flight fences",
			m_ImageAvailableSemaphores.size(), m_RenderFinishedSemaphores.size(), m_InFlightFences.size());
		spdlog::debug("Synchronization objects created");
    }

    ~SynchronizationObjects() 
    {
        for (size_t i = 0; i < m_MaxFramesInFlight; i++) {
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }
		spdlog::debug("Synchronization objects destroyed");
    }

    const VkSemaphore* getImageAvailableSemaphore(size_t index) const 
    {
		return& m_ImageAvailableSemaphores[index];
    }

    const VkSemaphore* getRenderFinishedSemaphore(size_t index) const 
    {
		return& m_RenderFinishedSemaphores[index];
    }

    const VkFence* getInFlightFence(size_t index) const 
    {
		return& m_InFlightFences[index];
    }

private:
    VkDevice m_Device;
    size_t m_MaxFramesInFlight;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
};
