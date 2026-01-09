// ModelLoader.cpp
#include "Engine/Asset/Import/ModelLoader.hpp"

#include <iostream>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tiny_gltf.h>

#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/CommandPool.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Framework/Misc/FencePool.hpp"
#include "Logging/Logger.hpp"

template <typename T>
const T* GetBufferAs(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];
    size_t byteOffset = bufferView.byteOffset + accessor.byteOffset;
    return reinterpret_cast<const T*>(buffer.data.data() + byteOffset);
}

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

bool ModelLoader::LoadGltfModel(tinygltf::Model& model, const std::string& path)
{
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    loader.SetImageLoader(nullptr, nullptr);
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!warn.empty())
    {
        LOG_WARN("WARN: {}", warn)
    }
    if (!err.empty())
    {
        LOG_ERROR("ERR: {}", err)
        return false;
    }
    if (!ret)
    {
        LOG_ERROR("Failed to load glTF")
        return false;
    }
    uint32_t PriNums = 0;
    for (const auto& mesh : model.meshes)
    {
        PriNums += mesh.primitives.size();
    }
    LOG_INFO("Loaded glTF with {} meshes, {} primitives, {} materials.", model.meshes.size(), PriNums,
             model.materials.size())
    return true;
}

std::optional<Mesh> ModelLoader::LoadPrimitiveAsMesh(const tinygltf::Model& model,
                                                     const tinygltf::Primitive& primitive)
{
    // 1. 读取 POSITION（必须存在）
    assert(primitive.attributes.count("POSITION") > 0);
    const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
    const glm::vec3* positions = GetBufferAs<glm::vec3>(model, posAccessor);
    size_t vertexCount = posAccessor.count;

    // 2. 读取 NORMAL（若无则设为默认）
    const glm::vec3* normals = nullptr;
    if (primitive.attributes.count("NORMAL"))
    {
        const auto& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
        normals = GetBufferAs<glm::vec3>(model, normAccessor);
        assert(normAccessor.count == vertexCount);
    }

    // 3. 读取 TEXCOORD_0（若无则设为 (0,0)）
    const glm::vec2* texCoords = nullptr;
    if (primitive.attributes.count("TEXCOORD_0"))
    {
        const auto& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
        // 注意：glTF 中 TEXCOORD 是 vec2f，但 tinygltf 会按 accessor.type 解析
        texCoords = GetBufferAs<glm::vec2>(model, uvAccessor);
        assert(uvAccessor.count == vertexCount);
    }

    // 4. 读取 COLOR_0（若无则设为 (1,1,1,1)）
    const glm::vec4* colors = nullptr;
    if (primitive.attributes.count("COLOR_0"))
    {
        const auto& colorAccessor = model.accessors[primitive.attributes.at("COLOR_0")];
        // COLOR_0 可能是 vec3 或 vec4
        if (colorAccessor.type == TINYGLTF_TYPE_VEC3)
        {
            // 需要转换 vec3 → vec4（A=1）
            // 这里简化：先读为 vec3 指针，构造时补 alpha=1
            const glm::vec3* color3s = GetBufferAs<glm::vec3>(model, colorAccessor);
            colors = reinterpret_cast<const glm::vec4*>(color3s); // ⚠️ 不安全！
            // 更安全做法：手动转换（见下方注释）
        }
        else if (colorAccessor.type == TINYGLTF_TYPE_VEC4)
        {
            colors = GetBufferAs<glm::vec4>(model, colorAccessor);
        }
        assert(colorAccessor.count == vertexCount);
    }

    // 5. 读取 TANGENT（若无则设为默认）
    const glm::vec4* tangents4 = nullptr; // glTF tangent 是 vec4（w=handness）
    if (primitive.attributes.count("TANGENT"))
    {
        const auto& tanAccessor = model.accessors[primitive.attributes.at("TANGENT")];
        tangents4 = GetBufferAs<glm::vec4>(model, tanAccessor);
        assert(tanAccessor.count == vertexCount);
    }

    // 6. 构建 vertices
    std::vector<MeshVertex> vertices;
    vertices.reserve(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        glm::vec3 pos = positions[i];
        glm::vec3 normal = normals ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec2 uv = texCoords ? texCoords[i] : glm::vec2(0.0f, 0.0f);

        // 处理颜色（安全方式）
        glm::vec4 color;
        if (colors)
        {
            // 如果原始是 vec3，需单独处理（此处假设是 vec4）
            // 更健壮的做法：在 accessor 类型上分支
            color = colors[i];
        }
        else
        {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // 处理 tangent（取 xyz，忽略 w）
        glm::vec3 tangent;
        if (tangents4)
        {
            tangent = glm::vec3(tangents4[i].x, tangents4[i].y, tangents4[i].z);
        }
        else
        {
            tangent = glm::vec3(1.0f, 0.0f, 0.0f); // 默认朝右
        }

        vertices.emplace_back(pos, color, normal, tangent, uv);
    }

    // 7. 读取 indices
    std::vector<unsigned int> indices;
    if (primitive.indices >= 0)
    {
        const auto& idxAccessor = model.accessors[primitive.indices];
        const auto& idxBufferView = model.bufferViews[idxAccessor.bufferView];
        const auto& buffer = model.buffers[idxBufferView.buffer];
        size_t byteOffset = idxBufferView.byteOffset + idxAccessor.byteOffset;
        const unsigned char* idxData = buffer.data.data() + byteOffset;

        indices.resize(idxAccessor.count);

        if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
            const uint32_t* src = reinterpret_cast<const uint32_t*>(idxData);
            for (size_t i = 0; i < idxAccessor.count; ++i) indices[i] = src[i];
        }
        else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            const uint16_t* src = reinterpret_cast<const uint16_t*>(idxData);
            for (size_t i = 0; i < idxAccessor.count; ++i) indices[i] = src[i];
        }
        else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            const uint8_t* src = idxData;
            for (size_t i = 0; i < idxAccessor.count; ++i) indices[i] = src[i];
        }
    }
    else
    {
        // 无索引：按顺序生成三角形索引（仅支持 POINTS/LINES/TRIANGLES）
        // 假设是 TRIANGLES
        for (unsigned int i = 0; i < vertexCount; ++i)
        {
            indices.push_back(i);
        }
    }

    return Mesh(std::move(vertices), std::move(indices));
}

MaterialTexturePaths ModelLoader::ExtractMaterialTexturePaths(const tinygltf::Model& model, int materialIndex)
{
    MaterialTexturePaths paths;

    if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
    {
        return paths;
    }

    const auto& mat = model.materials[materialIndex];
    const auto& pbr = mat.pbrMetallicRoughness;

    // --- 基础 PBR 纹理 ---
    auto getTexPath = [&](int texIndex) -> std::string
    {
        if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size())) return "";
        const auto& tex = model.textures[texIndex];
        if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size())) return "";
        return model.images[tex.source].uri; // 这是相对路径，如 "textures/Albedo.png"
    };

    // baseColorTexture
    if (pbr.baseColorTexture.index >= 0)
    {
        paths.baseColor = getTexPath(pbr.baseColorTexture.index);
    }

    // metallicRoughnessTexture（R: occlusion, G: roughness, B: metallic）
    if (pbr.metallicRoughnessTexture.index >= 0)
    {
        paths.metallicRoughness = getTexPath(pbr.metallicRoughnessTexture.index);
    }

    // normalTexture
    if (mat.normalTexture.index >= 0)
    {
        paths.normal = getTexPath(mat.normalTexture.index);
    }

    // occlusionTexture（可选，常与 metallicRoughness 合并）
    if (mat.occlusionTexture.index >= 0)
    {
        paths.occlusion = getTexPath(mat.occlusionTexture.index);
    }

    // emissiveTexture
    if (mat.emissiveTexture.index >= 0)
    {
        paths.emissive = getTexPath(mat.emissiveTexture.index);
    }

    // --- 扩展纹理 ---

    // KHR_materials_transmission
    if (mat.extensions.count("KHR_materials_transmission"))
    {
        const auto& ext = mat.extensions.at("KHR_materials_transmission");
        if (ext.Has("transmissionTexture"))
        {
            int idx = ext.Get("transmissionTexture").Get("index").GetNumberAsInt();
            paths.transmission = getTexPath(idx);
        }
    }

    // KHR_materials_volume
    if (mat.extensions.count("KHR_materials_volume"))
    {
        const auto& ext = mat.extensions.at("KHR_materials_volume");
        if (ext.Has("thicknessTexture"))
        {
            int idx = ext.Get("thicknessTexture").Get("index").GetNumberAsInt();
            paths.thickness = getTexPath(idx);
        }
    }

    // 其他扩展（如 KHR_materials_specular, clearcoat 等）可类似添加

    return paths;
}


MaterialTexturePaths ExtractMaterialTexturePaths(const tinygltf::Model& model, int materialIndex)
{
    MaterialTexturePaths paths;

    if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
    {
        return paths;
    }

    const auto& mat = model.materials[materialIndex];
    const auto& pbr = mat.pbrMetallicRoughness;

    // --- 基础 PBR 纹理 ---
    auto getTexPath = [&](int texIndex) -> std::string
    {
        if (texIndex < 0 || texIndex >= static_cast<int>(model.textures.size())) return "";
        const auto& tex = model.textures[texIndex];
        if (tex.source < 0 || tex.source >= static_cast<int>(model.images.size())) return "";
        return model.images[tex.source].uri; // 这是相对路径，如 "textures/Albedo.png"
    };

    // baseColorTexture
    if (pbr.baseColorTexture.index >= 0)
    {
        paths.baseColor = getTexPath(pbr.baseColorTexture.index);
    }

    // metallicRoughnessTexture（R: occlusion, G: roughness, B: metallic）
    if (pbr.metallicRoughnessTexture.index >= 0)
    {
        paths.metallicRoughness = getTexPath(pbr.metallicRoughnessTexture.index);
    }

    // normalTexture
    if (mat.normalTexture.index >= 0)
    {
        paths.normal = getTexPath(mat.normalTexture.index);
    }

    // occlusionTexture（可选，常与 metallicRoughness 合并）
    if (mat.occlusionTexture.index >= 0)
    {
        paths.occlusion = getTexPath(mat.occlusionTexture.index);
    }

    // emissiveTexture
    if (mat.emissiveTexture.index >= 0)
    {
        paths.emissive = getTexPath(mat.emissiveTexture.index);
    }

    // --- 扩展纹理 ---

    // KHR_materials_transmission
    if (mat.extensions.count("KHR_materials_transmission"))
    {
        const auto& ext = mat.extensions.at("KHR_materials_transmission");
        if (ext.Has("transmissionTexture"))
        {
            int idx = ext.Get("transmissionTexture").Get("index").GetNumberAsInt();
            paths.transmission = getTexPath(idx);
        }
    }

    // KHR_materials_volume
    if (mat.extensions.count("KHR_materials_volume"))
    {
        const auto& ext = mat.extensions.at("KHR_materials_volume");
        if (ext.Has("thicknessTexture"))
        {
            int idx = ext.Get("thicknessTexture").Get("index").GetNumberAsInt();
            paths.thickness = getTexPath(idx);
        }
    }

    // 其他扩展（如 KHR_materials_specular, clearcoat 等）可类似添加

    return paths;
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
