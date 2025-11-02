// ModelLoader.cpp
#include "Engine/Asset/Import/ModelLoader.hpp"

#include <iostream>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/CommandPool.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Framework/Misc/FencePool.hpp"
#include "Logging/Logger.hpp"

Mesh Model::MergeMeshes() const
{
    std::vector<MeshVertex> combinedVertices;
    std::vector<unsigned int> combinedIndices;


    unsigned int vertexOffset = 0;

    for (const auto& mesh : meshes)
    {
        // 1. 合并顶点数据
        // 直接将当前 mesh 的所有顶点追加到 combinedVertices 的末尾
        combinedVertices.insert(combinedVertices.end(), mesh.vertices.begin(), mesh.vertices.end());

        // 2. 合并索引数据，并应用偏移量
        for (unsigned int index : mesh.indices)
        {
            combinedIndices.push_back(index + vertexOffset);
        }

        // 4. 更新下一个网格的顶点偏移量
        vertexOffset += mesh.vertices.size();
    }

    // 返回合并后的新 Mesh 对象
    return Mesh(combinedVertices, combinedIndices);
}

std::optional<Model> ModelLoader::LoadModel(const std::string& path)
{
    Assimp::Importer importer;
    // aiProcess_CalcTangentSpace: 计算切线空间，用于法线贴图
    // aiProcess_Triangulate: 将所有面转换为三角形
    // aiProcess_GenSmoothNormals: 如果模型没有法线，则生成平滑法线
    const aiScene* scene = importer.ReadFile(path,
                                             aiProcess_Triangulate |
                                             aiProcess_GenSmoothNormals |
                                             aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("ERROR::ASSIMP:: {}", importer.GetErrorString())
        return std::nullopt;
    }
    LOG_INFO("Loaded model {} successfully", path)

    m_directory = path.substr(0, path.find_last_of('/'));

    Model model;
    ProcessNode(scene->mRootNode, scene, model);

    return model;
}

std::optional<Mesh> ModelLoader::LoadAsSingleMesh(const std::string& path)
{
    std::optional<Model> model = LoadModel(path);

    if (!model)
    {
        return std::nullopt;
    }

    if (model->meshes.empty())
    {
        return std::nullopt;
    }
    return model->MergeMeshes();
}

bool ModelLoader::MeshToBuffer(vkb::VulkanDevice& device, scene::MeshData& mesh_data, const Mesh& mesh,
                               VkBufferUsageFlags additional_buffer_usage_flags)
{
    auto& queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
    auto command_buffer = device.request_command_buffer();
    command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // === 1. vertex buffer ===
    VkDeviceSize vertexBufferSize = mesh.vertices.size() * sizeof(MeshVertex);
    vkb::Buffer stagingVertexBuffer = vkb::Buffer::create_staging_buffer(device, mesh.vertices);

    VkBufferUsageFlags vertexBufferUsage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        additional_buffer_usage_flags;

    vkb::Buffer vertexBuffer{
        device,
        vertexBufferSize,
        vertexBufferUsage,
        VMA_MEMORY_USAGE_GPU_ONLY
    };

    command_buffer->copy_buffer(stagingVertexBuffer, vertexBuffer, vertexBufferSize);

    // Insert into vertex_buffers (using "Vertex" as the key for easy subsequent binding)
    mesh_data.vertex_buffers.try_emplace("Vertex", std::move(vertexBuffer));

    // === 2. Index buffer ===
    VkDeviceSize indexBufferSize = mesh.indices.size() * sizeof(uint32_t);
    vkb::Buffer stagingIndexBuffer = vkb::Buffer::create_staging_buffer(device, mesh.indices);

    VkBufferUsageFlags indexBufferUsage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        additional_buffer_usage_flags;

    mesh_data.index_buffer = std::make_unique<vkb::Buffer>(
        device,
        indexBufferSize,
        indexBufferUsage,
        VMA_MEMORY_USAGE_GPU_ONLY);

    command_buffer->copy_buffer(stagingIndexBuffer, *mesh_data.index_buffer, indexBufferSize);

    // === 3. Populate the MeshData metadata ===
    mesh_data.vertices_count = static_cast<uint32_t>(mesh.vertices.size());
    mesh_data.index_count = static_cast<uint32_t>(mesh.indices.size());
    mesh_data.index_type = VK_INDEX_TYPE_UINT32;
    mesh_data.index_buffer_offset = 0; // Usually 0, unless you perform buffer offset drawing

    // === 4. Set vertex attribute description (strided layout) ===
    mesh_data.vertex_attributes["position"] = scene::MeshData::VertexAttribute{
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(MeshVertex, pos),
        "Vertex"
    };

    mesh_data.vertex_attributes["normal"] = scene::MeshData::VertexAttribute{
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(MeshVertex, normal),
        "Vertex"
    };

    mesh_data.vertex_attributes["texcoord_0"] = scene::MeshData::VertexAttribute{
        VK_FORMAT_R32G32_SFLOAT,
        offsetof(MeshVertex, texCoord),
        "Vertex"
    };

    mesh_data.vertex_attributes["color"] = scene::MeshData::VertexAttribute{
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(MeshVertex, color),
        "Vertex"
    };

    mesh_data.vertex_attributes["tangent"] = scene::MeshData::VertexAttribute{
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(MeshVertex, tangent),
        "Vertex"
    };

    // === 5. Set vertex binding description ===
    mesh_data.vertex_buffer_bindings["Vertex"] = scene::MeshData::VertexBufferBinding{
        &mesh_data.vertex_buffers.at("Vertex"),
        sizeof(MeshVertex),
        VK_VERTEX_INPUT_RATE_VERTEX
    };

    command_buffer->end();
    auto fence = device.request_fence();
    queue.submit(*command_buffer, fence);
    device.get_fence_pool().wait();
    device.get_fence_pool().reset();
    device.get_command_pool().reset_pool();
    return true;
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, Model& model)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(ProcessMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene, model);
    }
}

Mesh ModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        MeshVertex vertex;

        // Pos
        vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        // Normal
        if (mesh->HasNormals())
        {
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else
        {
            vertex.normal = glm::vec3(0.0f);
        }

        // UV[0]
        if (mesh->mTextureCoords[0])
        {
            vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertex.texCoord = glm::vec2(0.0f);
        }

        // Tangent
        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        }
        else
        {
            vertex.tangent = glm::vec3(0.0f);
        }

        // VertexColor
        if (mesh->mColors[0])
        {
            vertex.color = glm::vec4(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b,
                                     mesh->mColors[0][i].a);
        }
        else
        {
            vertex.color = glm::vec4(1.0f);
        }

        vertices.push_back(vertex);
    }

    // index
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    return Mesh(vertices, indices);
}
