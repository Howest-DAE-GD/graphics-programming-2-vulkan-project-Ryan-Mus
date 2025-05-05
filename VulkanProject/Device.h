#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <string>
#include <fstream>
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

class PhysicalDevice;
class Device 
{
public:
    Device(VkDevice device, VkQueue graphicsQueue, VkQueue presentQueue);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    VkDevice get() const;
    VkQueue getGraphicsQueue() const;
    VkQueue getPresentQueue() const;
private:
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
};
