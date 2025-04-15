#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

class SynchronizationObjects 
{
public:
    SynchronizationObjects(VkDevice device, size_t maxFramesInFlight)
        : device_(device), maxFramesInFlight_(maxFramesInFlight) {
        imageAvailableSemaphores_.resize(maxFramesInFlight_);
        renderFinishedSemaphores_.resize(maxFramesInFlight_);
        inFlightFences_.resize(maxFramesInFlight_);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < maxFramesInFlight_; i++) {
            if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
                vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    ~SynchronizationObjects() {
        for (size_t i = 0; i < maxFramesInFlight_; i++) {
            vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
            vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
            vkDestroyFence(device_, inFlightFences_[i], nullptr);
        }
    }

    const VkSemaphore* getImageAvailableSemaphore(size_t index) const 
    {
		return& imageAvailableSemaphores_[index];

    }

    const VkSemaphore* getRenderFinishedSemaphore(size_t index) const 
    {
		return& renderFinishedSemaphores_[index];
    }

    const VkFence* getInFlightFence(size_t index) const 
    {
		return& inFlightFences_[index];
    }

private:
    VkDevice device_;
    size_t maxFramesInFlight_;
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
};
