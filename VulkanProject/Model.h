#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Buffer.h"
#include "CommandPool.h"
#include "Device.h"
#include "Texture.h"
#include "Material.h"

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;    // Added tangent
    glm::vec3 bitangent;  // Added bitangent

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    static std::vector<VkVertexInputAttributeDescription> getDepthAttributeDescriptions();

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
            seed ^= vec3Hasher(vertex.normal) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= vec3Hasher(vertex.tangent) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= vec3Hasher(vertex.bitangent) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

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
    std::pair<glm::vec3, glm::vec3> getAABB() const 
    {
        return { m_BoundingBoxMin, m_BoundingBoxMax };
    }

private:
    void processNode(aiNode* node, const aiScene* scene, std::unordered_map<Vertex, uint32_t>& uniqueVertices, glm::mat4 parentTransform);
    void processMesh(aiMesh* mesh, const aiScene* scene, std::unordered_map<Vertex, uint32_t>& uniqueVertices, glm::mat4 transform, glm::vec3 scale);

    VmaAllocator m_Allocator;
    Device* m_pDevice;
    PhysicalDevice* m_pPhysicalDevice;
    CommandPool* m_pCommandPool;
    std::string m_ModelPath;
    std::string m_Directory;

    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    Buffer* m_pVertexBuffer;
    Buffer* m_pIndexBuffer;

    std::vector<Submesh> m_Submeshes;
    std::vector<Material*> m_Materials;
	glm::vec3 m_BoundingBoxMin;
	glm::vec3 m_BoundingBoxMax;
};

