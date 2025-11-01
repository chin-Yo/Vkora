#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <volk.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "Engine/SceneGraph/Components/SubMesh.hpp"

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std
{
    template <>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(
                vertex.texCoord) << 1);
        }
    };
}

namespace scene
{
    class SubMesh;
}

namespace vkb
{
    class VulkanDevice;
}

namespace asset
{
    class ObjLoader
    {
    public:
        ObjLoader(vkb::VulkanDevice& device);
        ~ObjLoader();

        std::unique_ptr<scene::SubMesh> ReadModelFromFile(const std::string& file_name, uint32_t index,
                                                          bool storage_buffer = false,
                                                          VkBufferUsageFlags additional_buffer_usage_flags = 0);

        void ReadMeshDataFromFile(scene::MeshData& mesh_data, const std::string& file_name,
                                  VkBufferUsageFlags additional_buffer_usage_flags = 0);

    private:
        std::vector<Vertex> vertices;

        std::vector<uint32_t> indices;

        vkb::VulkanDevice& device;
    };
}
