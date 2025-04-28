#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <spdlog/spdlog.h>
#include "PhysicalDevice.h"

Model::Model(VmaAllocator allocator, Device* device, PhysicalDevice* pPhysicalDevice, CommandPool* commandPool, const std::string& modelPath)
    : m_Allocator(allocator), m_pDevice(device),m_pPhysicalDevice{pPhysicalDevice}, m_pCommandPool(commandPool), m_ModelPath(modelPath),
    m_pVertexBuffer(nullptr), m_pIndexBuffer(nullptr) 
{
    spdlog::debug("Model created with path: {}", m_ModelPath);
}

Model::~Model() 
{
    delete m_pVertexBuffer;
    delete m_pIndexBuffer;

    for (Material* texture : m_Materials)
    {
        delete texture;
    }
	spdlog::debug("Model destroyed");
}

//helper function
std::string normalizePath(const std::string& path) 
{
    std::string normalizedPath = path;
    // Replace backslashes with forward slashes
    for (auto& c : normalizedPath) 
    {
        if (c == '\\') {
            c = '/';
        }
    }
    return normalizedPath;
}

void Model::loadModel()
{
    spdlog::debug("Loading model from path: {}", m_ModelPath);
    std::string base_dir = "models/";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, normalizePath(m_ModelPath).c_str(), base_dir.c_str());
    if (!err.empty())
    {
        spdlog::error("TinyObjLoader: {}", err);
    }
    if (!ret)
    {
        throw std::runtime_error("Failed to load model");
    }

    // Load materials
    m_Materials.resize(materials.size());

    for (size_t i = 0; i < materials.size(); ++i)
    {
        std::string diffuseTexturePath = materials[i].diffuse_texname;
        std::string specularTexturePath = materials[i].specular_texname;
        std::string alphaTexturePath = materials[i].alpha_texname;

        if (diffuseTexturePath.empty())
        {
            diffuseTexturePath = "default_white.png";
        }
        if (specularTexturePath.empty())
        {
            specularTexturePath = "default_black.png";
        }
        if (alphaTexturePath.empty())
        {
            alphaTexturePath = "default_white.png";
        }

        m_Materials[i] = new Material();
        m_Materials[i]->pDiffuseTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool,
            base_dir + normalizePath(diffuseTexturePath), m_pPhysicalDevice->get());
        m_Materials[i]->pSpecularTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool,
            base_dir + normalizePath(specularTexturePath), m_pPhysicalDevice->get());
        m_Materials[i]->pAlphaTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool,
            base_dir + normalizePath(alphaTexturePath), m_pPhysicalDevice->get());
    }

    m_Vertices.clear();
    m_Indices.clear();
    m_Submeshes.clear();

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    uint32_t globalIndexOffset = 0;

    // Loop over shapes (meshes)
    for (const auto& shape : shapes)
    {
        size_t indexOffset = 0;
        size_t numFaces = shape.mesh.num_face_vertices.size();

        glm::vec3 bboxMin = { FLT_MAX, FLT_MAX, FLT_MAX };
        glm::vec3 bboxMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        // Map to store material to indices mapping for this shape
        std::unordered_map<int, std::vector<uint32_t>> materialToIndices;

        // Loop over faces
        for (size_t f = 0; f < numFaces; f++)
        {
            int numFaceVertices = shape.mesh.num_face_vertices[f];
            assert(numFaceVertices == 3);  // Only support triangles
            int materialId = shape.mesh.material_ids[f];

            // Loop over vertices in the face
            for (size_t v = 0; v < numFaceVertices; v++)
            {
                tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                // Update bounding box
                bboxMin = glm::min(bboxMin, vertex.pos);
                bboxMax = glm::max(bboxMax, vertex.pos);

                if (idx.texcoord_index >= 0)
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1] // Flip V coordinate
                    };
                }
                else
                {
                    vertex.texCoord = { 0.0f, 0.0f };
                }

                if (idx.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    };
                }
                else
                {
                    vertex.normal = { 0.0f, 0.0f, 1.0f }; // Default normal
                }

                // Check if vertex is already in uniqueVertices
                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                    m_Vertices.push_back(vertex);
                }

                uint32_t index = uniqueVertices[vertex];
                materialToIndices[materialId].push_back(index);
            }
            indexOffset += numFaceVertices;
        }

        // For each material used in this shape, create a submesh
        for (const auto& pair : materialToIndices)
        {
            int materialId = pair.first;
            const std::vector<uint32_t>& indices = pair.second;

            Submesh submesh{};
            submesh.indexStart = static_cast<uint32_t>(m_Indices.size());
            submesh.indexCount = static_cast<uint32_t>(indices.size());
            submesh.materialIndex = static_cast<uint32_t>(materialId);
            submesh.bboxMin = bboxMin;
            submesh.bboxMax = bboxMax;

            m_Submeshes.push_back(submesh);
            m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());
        }
    }

    spdlog::debug("Loaded model with {} vertices, {} indices, and {} materials.", m_Vertices.size(), m_Indices.size(), m_Materials.size());
}


void Model::createVertexBuffer() 
{
    spdlog::debug("Creating vertex buffer");
    VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

    Buffer stagingBuffer(
        m_Allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void* data = stagingBuffer.map();
    memcpy(data, m_Vertices.data(), (size_t)bufferSize);

    m_pVertexBuffer = new Buffer(
        m_Allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(m_pCommandPool, m_pDevice->getGraphicsQueue(), m_pVertexBuffer);

    stagingBuffer.unmap();
    stagingBuffer.flush();

    spdlog::debug("Vertex buffer created size: {}", bufferSize);
}

void Model::createIndexBuffer() 
{
    spdlog::debug("Creating index buffer");
    VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

    Buffer stagingBuffer(
        m_Allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void* data = stagingBuffer.map();
    memcpy(data, m_Indices.data(), (size_t)bufferSize);
   

    m_pIndexBuffer = new Buffer(
        m_Allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(m_pCommandPool, m_pDevice->getGraphicsQueue(), m_pIndexBuffer);

    stagingBuffer.unmap();
    stagingBuffer.flush();
    spdlog::debug("Index buffer size: {}", bufferSize);
    spdlog::debug("Index buffer created successfully");
}

VkBuffer Model::getVertexBuffer() const 
{
    return m_pVertexBuffer->get();
}

VkBuffer Model::getIndexBuffer() const 
{
    return m_pIndexBuffer->get();
}

size_t Model::getIndexCount() const 
{
    return m_Indices.size();
}

bool Vertex::operator==(const Vertex& other) const 
{
    return pos == other.pos && texCoord == other.texCoord;
}

VkVertexInputBindingDescription Vertex::getBindingDescription() 
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, normal);

    return attributeDescriptions;
}
std::vector<VkVertexInputAttributeDescription> Vertex::getDepthAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
};
