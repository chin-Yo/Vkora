#pragma once

#include <string>
#include <vector>
#include <map>
#include <volk.h>
// GLM for vector/matrix math
#include <optional>
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkb
{
    class VulkanDevice;
}

namespace scene
{
    struct MeshData;
}

// Assimp forward declarations to avoid including heavy headers in our public header
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;


struct MeshVertex
{
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec3 tangent;

    MeshVertex()
    {
        Reset();
    }

    MeshVertex(const glm::vec3& _pos,
               const glm::vec4& _color,
               const glm::vec3& _normal,
               const glm::vec3& _tangent,
               const glm::vec2& _texCoord)
        : pos(_pos)
          , color(_color)
          , normal(_normal)
          , tangent(_tangent)
          , texCoord(_texCoord)
    {
    }

    MeshVertex(float px, float py, float pz,
               float nx, float ny, float nz,
               float tx, float ty, float tz,
               float u, float v)
        : pos(px, py, pz)
          , color(1.0f, 1.0f, 1.0f, 1.0f)
          , normal(nx, ny, nz)
          , tangent(tx, ty, tz)
          , texCoord(u, v)
    {
    }

    MeshVertex(float px, float py, float pz,
               float cx, float cy, float cz, float cw,
               float nx, float ny, float nz,
               float tx, float ty, float tz,
               float u, float v)
        : pos(px, py, pz)
          , color(cx, cy, cz, cw)
          , normal(nx, ny, nz)
          , tangent(tx, ty, tz)
          , texCoord(u, v)
    {
    }

    bool operator==(const MeshVertex& other) const
    {
        return this->pos == other.pos &&
            this->color == other.color &&
            this->normal == other.normal &&
            this->tangent == other.tangent &&
            this->texCoord == other.texCoord;
    }

    void Reset()
    {
        this->pos = glm::vec3(0, 0, 0);
        this->color = glm::vec4(0, 0, 0, 1);
        this->normal = glm::vec3(0, 1, 0);
        this->tangent = glm::vec3(0, 0, 1);
        this->texCoord = glm::vec2(0, 0);
    }
};

class Mesh
{
public:
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices)
        : vertices(std::move(vertices)), indices(std::move(indices))
    {
    }
};

class Model
{
public:
    std::vector<Mesh> meshes;
    Mesh MergeMeshes() const;
};

struct MaterialTexturePaths
{
    std::string baseColor;
    std::string metallicRoughness; // 通常是 ORM 纹理（Occlusion, Roughness, Metallic）
    std::string normal;
    std::string emissive;
    std::string occlusion; // 虽然常和 metallicRoughness 合并，但 glTF 单独定义
    // 扩展
    std::string transmission;
    std::string thickness; // 来自 KHR_materials_volume
    // 可继续扩展...
};


class ModelLoader
{
public:
    static ModelLoader& GetInstance()
    {
        static ModelLoader instance;
        return instance;
    }

    ModelLoader(const ModelLoader&) = delete;
    ModelLoader& operator=(const ModelLoader&) = delete;

    std::optional<Model> LoadModel(const std::string& path);

    std::optional<Mesh> LoadAsSingleMesh(const std::string& path);

    bool LoadGltfModel(tinygltf::Model& model, const std::string& path);

    static std::optional<Mesh> LoadPrimitiveAsMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

    static MaterialTexturePaths ExtractMaterialTexturePaths(const tinygltf::Model& model, int materialIndex);

    static bool MeshToBuffer(vkb::VulkanDevice& device, scene::MeshData& mesh_data, const Mesh& mesh,
                             VkBufferUsageFlags additional_buffer_usage_flags = 0);

private:
    ModelLoader() = default;
    ~ModelLoader() = default;

    void ProcessNode(aiNode* node, const aiScene* scene, Model& model);

    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

    std::string m_directory; // 当前加载模型的目录
};
