#include "Model.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <spdlog/spdlog.h>
#include "PhysicalDevice.h"

Model::Model(VmaAllocator allocator, Device* device, PhysicalDevice* pPhysicalDevice, CommandPool* commandPool, const std::string& modelPath)
    : m_Allocator(allocator), m_pDevice(device), m_pPhysicalDevice(pPhysicalDevice), m_pCommandPool(commandPool), m_ModelPath(modelPath),
    m_pVertexBuffer(nullptr), m_pIndexBuffer(nullptr)
{
    spdlog::debug("Model created with path: {}", m_ModelPath);
}

Model::~Model()
{
    delete m_pVertexBuffer;
    delete m_pIndexBuffer;

    for (Material* material : m_Materials)
    {
        delete material;
    }
    spdlog::debug("Model destroyed");
}

void Model::loadModel()
{
    spdlog::debug("Loading model from path: {}", m_ModelPath);
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(m_ModelPath, aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_OptimizeMeshes |
        aiProcess_SortByPType);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        spdlog::error("Assimp Error: {}", importer.GetErrorString());
        throw std::runtime_error("Failed to load model");
    }

    m_Vertices.clear();
    m_Indices.clear();
    m_Submeshes.clear();
    m_Materials.clear();

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    m_Directory = m_ModelPath.substr(0, m_ModelPath.find_last_of('/'));

    processNode(scene->mRootNode, scene, uniqueVertices, glm::mat4(1.0f));


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
    memcpy(data, m_Vertices.data(), static_cast<size_t>(bufferSize));
    stagingBuffer.unmap();
    stagingBuffer.flush();

    m_pVertexBuffer = new Buffer(
        m_Allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(m_pCommandPool, m_pDevice->getGraphicsQueue(), m_pVertexBuffer);

    spdlog::debug("Vertex buffer created with size: {}", bufferSize);
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
    memcpy(data, m_Indices.data(), static_cast<size_t>(bufferSize));
    stagingBuffer.unmap();
    stagingBuffer.flush();

    m_pIndexBuffer = new Buffer(
        m_Allocator,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    stagingBuffer.copyTo(m_pCommandPool, m_pDevice->getGraphicsQueue(), m_pIndexBuffer);

    spdlog::debug("Index buffer created with size: {}", bufferSize);
    spdlog::debug("Index buffer created successfully");
}


void Model::processNode(aiNode* node, const aiScene* scene, std::unordered_map<Vertex, uint32_t>& uniqueVertices, glm::mat4 parentTransform)
{
    // Convert Assimp's aiMatrix4x4 to glm::mat4
    aiMatrix4x4 nodeTransformation = node->mTransformation;
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&nodeTransformation.a1));

    // Decompose the transformation matrix to extract scale
    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;
    glm::quat rotation;
    glm::decompose(nodeTransform, scale, rotation, translation, skew, perspective);

    // Combine the scale with the parent's scale
    glm::vec3 combinedScale = glm::vec3(parentTransform[0][0], parentTransform[1][1], parentTransform[2][2]) * scale;

    // Compute the current transformation by combining with the parent
    glm::mat4 currentTransform = parentTransform * nodeTransform;

    // Process all the node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, uniqueVertices, currentTransform, combinedScale);
    }

    // Recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, uniqueVertices, currentTransform);
    }
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene, std::unordered_map<Vertex, uint32_t>& uniqueVertices, glm::mat4 transform, glm::vec3 scale)
{
    Submesh submesh{};
    glm::vec3 bboxMin(FLT_MAX);
    glm::vec3 bboxMax(-FLT_MAX);

    submesh.indexStart = static_cast<uint32_t>(m_Indices.size());

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex{};

        // Apply the transformation and scale to the vertex position
        glm::vec4 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        pos = transform * pos;
        vertex.pos = glm::vec3(pos);

        bboxMin = glm::min(bboxMin, vertex.pos);
        bboxMax = glm::max(bboxMax, vertex.pos);

        if (mesh->HasNormals())
        {
            // Transform the normal
            glm::vec4 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
            normal = transform * normal;
            vertex.normal = glm::normalize(glm::vec3(normal));
        }

        if (mesh->mTextureCoords[0])
        {
            vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }

        if (mesh->HasTangentsAndBitangents())
        {
            glm::vec3 tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            glm::vec3 bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

            // Optional: ensure tangent is orthogonal to normal
            tangent = glm::normalize(tangent - vertex.normal * glm::dot(vertex.normal, tangent));

            // Optional: recompute bitangent to ensure consistency
            bitangent = glm::normalize(glm::cross(vertex.normal, tangent));

            vertex.tangent = tangent;
            vertex.bitangent = bitangent;
        }

        // Check if vertex is already in uniqueVertices
        if (uniqueVertices.count(vertex) == 0)
        {
            uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
            m_Vertices.push_back(vertex);
        }
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            uint32_t meshIndex = face.mIndices[j];

            // Construct the vertex from the mesh data directly
            Vertex vertex{};
            vertex.pos = glm::vec3(mesh->mVertices[meshIndex].x, mesh->mVertices[meshIndex].y, mesh->mVertices[meshIndex].z) * scale;

            if (mesh->HasNormals())
            {
                vertex.normal = glm::vec3(mesh->mNormals[meshIndex].x, mesh->mNormals[meshIndex].y, mesh->mNormals[meshIndex].z);
            }

            if (mesh->mTextureCoords[0])
            {
                vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][meshIndex].x, mesh->mTextureCoords[0][meshIndex].y);
            }

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.tangent = glm::vec3(mesh->mTangents[meshIndex].x, mesh->mTangents[meshIndex].y, mesh->mTangents[meshIndex].z);
                vertex.bitangent = glm::vec3(mesh->mBitangents[meshIndex].x, mesh->mBitangents[meshIndex].y, mesh->mBitangents[meshIndex].z);
            }

            // Get or create the optimized index for this vertex
            uint32_t optimizedIndex;
            if (uniqueVertices.count(vertex) == 0)
            {
                optimizedIndex = static_cast<uint32_t>(m_Vertices.size());
                uniqueVertices[vertex] = optimizedIndex;
                m_Vertices.push_back(vertex);
            }
            else
            {
                optimizedIndex = uniqueVertices[vertex];
            }

            m_Indices.push_back(optimizedIndex);
        }
    }

    submesh.indexCount = static_cast<uint32_t>(m_Indices.size()) - submesh.indexStart;
    submesh.bboxMin = bboxMin;
    submesh.bboxMax = bboxMax;

    // Process material (existing code)
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
        Material* material = new Material();

        // Albedo texture
        aiString albedoPath;
        if (aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &albedoPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &albedoPath) == AI_SUCCESS)
        {
            std::string fullPath = m_Directory + "/" + albedoPath.C_Str();
            material->pDiffuseTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, fullPath, m_pPhysicalDevice->get());
        }
        else
        {
            material->pDiffuseTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, "models/default_white.png", m_pPhysicalDevice->get());
        }

        // Normal texture
        aiString normalPath;
        if (aiMat->GetTexture(aiTextureType_NORMAL_CAMERA, 0, &normalPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_NORMALS, 0, &normalPath) == AI_SUCCESS ||
            aiMat->GetTexture(aiTextureType_HEIGHT, 0, &normalPath) == AI_SUCCESS)
        {
            std::string fullPath = m_Directory + "/" + normalPath.C_Str();
            material->pNormalTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, fullPath, m_pPhysicalDevice->get(), Texture::Format::UNORM);
        }
        else
        {
            material->pNormalTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, "models/default_black.png", m_pPhysicalDevice->get(), Texture::Format::UNORM);
        }

        // Metallic-Roughness texture
        aiString mrPath;
        if (aiMat->GetTexture(aiTextureType_UNKNOWN, 0, &mrPath) == AI_SUCCESS)
        {
            std::string fullPath = m_Directory + "/" + mrPath.C_Str();
            material->pMetallicRoughnessTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, fullPath, m_pPhysicalDevice->get());
        }
        else
        {
            material->pMetallicRoughnessTexture = new Texture(m_pDevice, m_Allocator, m_pCommandPool, "models/default_black.png", m_pPhysicalDevice->get());
        }

        m_Materials.push_back(material);
        submesh.materialIndex = static_cast<uint32_t>(m_Materials.size() - 1);
    }

    m_Submeshes.push_back(submesh);
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
    return pos == other.pos &&
        texCoord == other.texCoord &&
        normal == other.normal &&
        tangent == other.tangent &&
        bitangent == other.bitangent;
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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

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

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, tangent);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(Vertex, bitangent);

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
}
