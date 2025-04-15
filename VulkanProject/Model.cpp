// Model.cpp
#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <spdlog/spdlog.h>

Model::Model(VmaAllocator allocator, Device* device, CommandPool* commandPool, const std::string& modelPath)
    : allocator_(allocator), device_(device), commandPool_(commandPool), modelPath_(modelPath),
    vertexBuffer_(nullptr), indexBuffer_(nullptr) {
    spdlog::info("Model created with path: {}", modelPath_);
}

Model::~Model() {
    spdlog::info("Destroying Model");
    delete vertexBuffer_;
    delete indexBuffer_;
}

void Model::loadModel() {
    spdlog::info("Loading model from path: {}", modelPath_);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath_.c_str())) {
        spdlog::error("Failed to load model: {}", err);
        throw std::runtime_error(err);
    }

    spdlog::info("Model loaded successfully");

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.texcoord_index >= 0) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            else {
                vertex.texCoord = { 0.0f, 0.0f };
            }

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
                vertices_.push_back(vertex);
            }

            indices_.push_back(uniqueVertices[vertex]);
        }
    }

    spdlog::info("Model vertices and indices loaded: {} vertices, {} indices", vertices_.size(), indices_.size());
}

void Model::createVertexBuffer() {
    spdlog::info("Creating vertex buffer");
    VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

    Buffer stagingBuffer(
        allocator_,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void* data = stagingBuffer.map();
    memcpy(data, vertices_.data(), (size_t)bufferSize);

 //   // Log first vertex position
 //   auto* vertexData = static_cast<Vertex*>(data);
	//for (size_t i = 0; i < vertices_.size(); ++i) {
	//	spdlog::info("Vertex {}: x={}, y={}, z={}", i, vertexData[i].pos.x, vertexData[i].pos.y, vertexData[i].pos.z);
	//}
    //spdlog::info("First vertex position: x={}, y={}, z={}", vertexData[0].pos.x, vertexData[0].pos.y, vertexData[0].pos.z);

    

    vertexBuffer_ = new Buffer(
        allocator_,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(commandPool_, device_->getGraphicsQueue(), vertexBuffer_);

    stagingBuffer.unmap();
    stagingBuffer.flush();

    spdlog::info("Vertex buffer size: {}", bufferSize);
	spdlog::info("Vertex buffer created successfully");
}

void Model::createIndexBuffer() {
    spdlog::info("Creating index buffer");
    VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

    Buffer stagingBuffer(
        allocator_,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );

    void* data = stagingBuffer.map();
    memcpy(data, indices_.data(), (size_t)bufferSize);
   

    indexBuffer_ = new Buffer(
        allocator_,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(commandPool_, device_->getGraphicsQueue(), indexBuffer_);

    stagingBuffer.unmap();
    stagingBuffer.flush();
    spdlog::info("Index buffer size: {}", bufferSize);
    spdlog::info("Index buffer created successfully");
}

VkBuffer Model::getVertexBuffer() const {
    return vertexBuffer_->get();
}

VkBuffer Model::getIndexBuffer() const {
    return indexBuffer_->get();
}

size_t Model::getIndexCount() const {
    return indices_.size();
}

// Implement Vertex methods and hash function...


bool Vertex::operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
}

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
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