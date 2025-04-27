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
#include "Texture.h"
#include "Material.h"

struct Vertex 
{
    glm::vec3 pos;
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
            seed ^= vec2Hasher(vertex.texCoord) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

struct Submesh
{
    uint32_t indexStart;
    uint32_t indexCount;
	uint16_t materialIndex;

	glm::vec3 bboxMin;
	glm::vec3 bboxMax;
};

class PhysicalDevice;
class Model 
{
public:
    Model(VmaAllocator allocator, Device* pDevice, PhysicalDevice* pPhysicalDevice, CommandPool* pCommandPool, const std::string& modelPath);
    ~Model();

    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();

    VkBuffer getVertexBuffer() const;
    VkBuffer getIndexBuffer() const;
    size_t getIndexCount() const;

	std::vector<Submesh> getSubmeshes() const { return m_Submeshes; }
	std::vector<Material*> getMaterials() const { return m_Materials; }
private:
    VmaAllocator m_Allocator;
    Device* m_pDevice;
	PhysicalDevice* m_pPhysicalDevice;
    CommandPool* m_pCommandPool;
    std::string m_ModelPath;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    Buffer* m_pVertexBuffer;
    Buffer* m_pIndexBuffer;

	std::vector<Submesh> m_Submeshes;
	std::vector<Material*> m_Materials;
};
