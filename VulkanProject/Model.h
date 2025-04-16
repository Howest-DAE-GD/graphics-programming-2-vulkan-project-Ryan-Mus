#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "Buffer.h"
#include "CommandPool.h"
#include "Device.h"

struct Vertex 
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

    bool operator==(const Vertex& other) const;
};

namespace std 
{
    template<> struct hash<Vertex> 
    {
        size_t operator()(const Vertex& vertex) const 
        {
            size_t seed = 0;
            hash<glm::vec3> vec3Hasher;
            hash<glm::vec2> vec2Hasher;

            seed ^= vec3Hasher(vertex.pos) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= vec3Hasher(vertex.color) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= vec2Hasher(vertex.texCoord) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

class Model 
{
public:
    Model(VmaAllocator allocator, Device* pDevice, CommandPool* pCommandPool, const std::string& modelPath);
    ~Model();

    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();

    VkBuffer getVertexBuffer() const;
    VkBuffer getIndexBuffer() const;
    size_t getIndexCount() const;

private:
    VmaAllocator m_Allocator;
    Device* m_pDevice;
    CommandPool* m_pCommandPool;
    std::string m_ModelPath;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    Buffer* m_pVertexBuffer;
    Buffer* m_pIndexBuffer;
};
