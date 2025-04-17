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

    for (Texture* texture : m_Textures)
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
    spdlog::info("Be patient, lots of duplicate materials are created because of tinyObj");
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    std::string base_dir = "models/";  // Base directory for the OBJ file
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_ModelPath.c_str(), base_dir.c_str()))
    {
        spdlog::error("Failed to load model: {}", err);
        throw std::runtime_error(err);
    }
    if (!err.empty()) {
        spdlog::warn("Warning: {}", err);
    }
    // Deduplicate materials by name
    std::unordered_map<std::string, int> materialNameToIndex;
    std::vector<tinyobj::material_t> uniqueMaterials;
    for (const auto& material : materials) {
        // Check if we've seen this material name before
        if (materialNameToIndex.find(material.name) == materialNameToIndex.end()) {
            // New material, add it to our unique list
            materialNameToIndex[material.name] = static_cast<int>(uniqueMaterials.size());
            uniqueMaterials.push_back(material);
        }
    }
    spdlog::info("Materials loaded: {} original materials deduped to {} unique materials",
        materials.size(), uniqueMaterials.size());
    // Replace original materials with deduplicated ones
    materials = std::move(uniqueMaterials);
    // Map to store material indices for shapes, translating from original to deduplicated indices
    std::vector<int> materialIndexMap(materials.size());
    for (size_t i = 0; i < materials.size(); ++i) {
        materialIndexMap[i] = materialNameToIndex[materials[i].name];
    }

    // Create a mapping from material ID to texture ID
    std::unordered_map<int, uint32_t> materialToTextureIndex;
    // Default texture for materials without diffuse textures
    uint32_t defaultTextureIndex = 0;
    bool hasDefaultTexture = false;

    // Iterate over materials to load textures
    for (size_t i = 0; i < materials.size(); ++i)
    {
        const auto& material = materials[i];
        if (!material.diffuse_texname.empty())
        {
            std::string texturePath = base_dir + normalizePath(material.diffuse_texname);
            Texture* texture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, texturePath, m_pPhysicalDevice->get());
            uint32_t textureIndex = static_cast<uint32_t>(m_Textures.size());
            m_Textures.push_back(texture);
            materialToTextureIndex[i] = textureIndex;
        }
        else
        {
            // For materials without diffuse textures, either use a default texture or create one
            if (!hasDefaultTexture) {
                // Create or load a default white texture
                Texture* defaultTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool,
                    base_dir + "default_white.png", m_pPhysicalDevice->get());
                defaultTextureIndex = static_cast<uint32_t>(m_Textures.size());
                m_Textures.push_back(defaultTexture);
                hasDefaultTexture = true;
            }
            materialToTextureIndex[i] = defaultTextureIndex;
        }
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes)
    {
        // Process faces (triangles)
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            // Get material ID for this face
            int materialId = shape.mesh.material_ids[f];
            // Get corresponding texture index
            uint32_t textureIndex = (materialId >= 0 && materialToTextureIndex.find(materialId) != materialToTextureIndex.end())
                ? materialToTextureIndex[materialId]
                : defaultTextureIndex;

            // Process vertices for this face
            size_t fv = size_t(shape.mesh.num_face_vertices[f]);
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                if (idx.texcoord_index >= 0) {
                    vertex.texCoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }
                else {
                    vertex.texCoord = { 0.0f, 0.0f };
                }

                vertex.color = { 1.0f, 1.0f, 1.0f };

                // Assign the texture index to this vertex
                vertex.texIndex = textureIndex;

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                    m_Vertices.push_back(vertex);
                }
                m_Indices.push_back(uniqueVertices[vertex]);
            }
            index_offset += fv;
        }
    }
    spdlog::info("Model vertices and indices loaded: {} vertices, {} indices, {} textures",
        m_Vertices.size(), m_Indices.size(), m_Textures.size());
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
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[3].offset = offsetof(Vertex, texIndex);

    return attributeDescriptions;
}