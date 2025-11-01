#include "Import/ObjLoader.hpp"

#include <tiny_obj_loader.h>
#include <unordered_map>

#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/CommandPool.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Framework/Misc/FencePool.hpp"
#include "Logging/Logger.hpp"

namespace asset
{
    ObjLoader::ObjLoader(vkb::VulkanDevice& device)
        : device(device)
    {
    }

    std::unique_ptr<scene::SubMesh> ObjLoader::ReadModelFromFile(const std::string& file_name, uint32_t index,
                                                                 bool storage_buffer,
                                                                 VkBufferUsageFlags additional_buffer_usage_flags)
    {
        std::unique_ptr<scene::SubMesh> sub_mesh = std::make_unique<scene::SubMesh>();

        auto& queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
        auto command_buffer = device.request_command_buffer();

        command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name.c_str()))
        {
            LOG_ERROR("Failed to load OBJ file: %s", err.c_str())
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        sub_mesh->vertices_count = vertices.size();
        sub_mesh->index_type = VK_INDEX_TYPE_UINT32;
        sub_mesh->index_count = indices.size();
        sub_mesh->index_buffer_offset = sizeof(Vertex);

        {
            vkb::Buffer stage_buffer = vkb::Buffer::create_staging_buffer(device, vertices);

            vkb::Buffer buffer{
                device,
                vertices.size() * sizeof(Vertex),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            };

            command_buffer->copy_buffer(stage_buffer, buffer, vertices.size() * sizeof(Vertex));

            auto pair = std::make_pair("vertex_buffer", std::move(buffer));
            sub_mesh->vertex_buffers.insert(std::move(pair));
        }

        vkb::Buffer stage_buffer = vkb::Buffer::create_staging_buffer(device, indices);

        sub_mesh->index_buffer = std::make_unique<vkb::Buffer>(device,
                                                               indices.size(),
                                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                               VMA_MEMORY_USAGE_GPU_ONLY);

        command_buffer->copy_buffer(stage_buffer, *sub_mesh->index_buffer, indices.size());

        command_buffer->end();

        queue.submit(*command_buffer, device.request_fence());

        device.get_fence_pool().wait();
        device.get_fence_pool().reset();
        device.get_command_pool().reset_pool();

        return std::move(sub_mesh);
    }

    bool ObjLoader::ReadMeshDataFromFile(
        scene::MeshData& mesh_data,
        const std::string& file_name,
        VkBufferUsageFlags additional_buffer_usage_flags)
    {
        mesh_data = {};

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_name.c_str()))
        {
            LOG_ERROR("Failed to load OBJ file '%s': %s", file_name.c_str(), err.c_str());
            return false;
        }

        if (!warn.empty())
        {
            LOG_WARN("OBJ loader warning: %s", warn.c_str());
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for (const auto& shape : shapes)
        {
            for (const auto& idx : shape.mesh.indices)
            {
                Vertex vertex{};

                // Position
                if (idx.vertex_index >= 0)
                {
                    vertex.pos = {
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    };
                }
                else
                {
                    vertex.pos = {0.0f, 0.0f, 0.0f};
                }

                // TexCoord (flip Y)
                if (idx.texcoord_index >= 0 && !attrib.texcoords.empty())
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }
                else
                {
                    vertex.texCoord = {0.0f, 0.0f};
                }

                // Color (optional, set to white)
                vertex.color = {1.0f, 1.0f, 1.0f};

                // Deduplicate
                auto iter = uniqueVertices.find(vertex);
                if (iter == uniqueVertices.end())
                {
                    uint32_t index = static_cast<uint32_t>(vertices.size());
                    uniqueVertices[vertex] = index;
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        if (vertices.empty() || indices.empty())
        {
            LOG_ERROR("Loaded mesh has no vertices or indices");
            return false;
        }

        auto& queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);
        auto command_buffer = device.request_command_buffer();
        command_buffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // === 1. vertex buffer ===
        VkDeviceSize vertexBufferSize = vertices.size() * sizeof(Vertex);
        vkb::Buffer stagingVertexBuffer = vkb::Buffer::create_staging_buffer(device, vertices);

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
        mesh_data.vertex_buffers["Vertex"] = std::move(vertexBuffer);

        // === 2. Index buffer ===
        VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
        vkb::Buffer stagingIndexBuffer = vkb::Buffer::create_staging_buffer(device, indices);

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
        mesh_data.vertices_count = static_cast<uint32_t>(vertices.size());
        mesh_data.index_count = static_cast<uint32_t>(indices.size());
        mesh_data.index_type = VK_INDEX_TYPE_UINT32;
        mesh_data.index_buffer_offset = 0; // Usually 0, unless you perform buffer offset drawing

        // === 4. Set vertex attribute description (strided layout) ===
        mesh_data.vertex_attributes["position"] = scene::MeshData::VertexAttribute{
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(Vertex, pos),
            "Vertex"
        };

        mesh_data.vertex_attributes["texcoord_0"] = scene::MeshData::VertexAttribute{
            VK_FORMAT_R32G32_SFLOAT,
            offsetof(Vertex, texCoord),
            "Vertex"
        };

        mesh_data.vertex_attributes["normal"] = scene::MeshData::VertexAttribute{
            VK_FORMAT_R32G32B32_SFLOAT,
            offsetof(Vertex, color),
            "Vertex"
        };

        // === 5. Set vertex binding description ===
        mesh_data.vertex_buffer_bindings["Vertex"] = scene::MeshData::VertexBufferBinding{
            &mesh_data.vertex_buffers.at("Vertex"),
            sizeof(Vertex),
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
}
