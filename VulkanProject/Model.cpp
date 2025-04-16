#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <spdlog/spdlog.h>

Model::Model(VmaAllocator allocator, Device* device, CommandPool* commandPool, const std::string& modelPath)
    : m_Allocator(allocator), m_pDevice(device), m_pCommandPool(commandPool), m_ModelPath(modelPath),
    m_pVertexBuffer(nullptr), m_pIndexBuffer(nullptr) 
{
    spdlog::debug("Model created with path: {}", m_ModelPath);
}

Model::~Model() 
{
    delete m_pVertexBuffer;
    delete m_pIndexBuffer;
	spdlog::debug("Model destroyed");
}

void Model::loadModel() {
    spdlog::debug("Loading model from path: {}", m_ModelPath);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_ModelPath.c_str()))
    {
        spdlog::error("Failed to load model: {}", err);
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) 
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = 
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.texcoord_index >= 0) 
            {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            else 
            {
                vertex.texCoord = { 0.0f, 0.0f };
            }

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) 
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                m_Vertices.push_back(vertex);
            }

            m_Indices.push_back(uniqueVertices[vertex]);
        }
    }
    spdlog::info("Model vertices and indices loaded: {} vertices, {} indices", m_Vertices.size(), m_Indices.size());
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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

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

    return attributeDescriptions;
}