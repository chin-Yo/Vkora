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

    void ObjLoader::ReadMeshDataFromFile(scene::MeshData& mesh_data, const std::string& file_name, uint32_t index,
        bool storage_buffer, VkBufferUsageFlags additional_buffer_usage_flags)
    {
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

        mesh_data.vertices_count = vertices.size();
        mesh_data.index_type = VK_INDEX_TYPE_UINT32;
        mesh_data.index_count = indices.size();
        mesh_data.index_buffer_offset = sizeof(Vertex);

        {
            vkb::Buffer stage_buffer = vkb::Buffer::create_staging_buffer(device, vertices);

            vkb::Buffer buffer{
                device,
                vertices.size() * sizeof(Vertex),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY
            };

            command_buffer->copy_buffer(stage_buffer, buffer, vertices.size() * sizeof(Vertex));

            auto pair = std::make_pair("main", std::move(buffer));
            mesh_data.vertex_buffers.insert(std::move(pair));
        }


        vkb::Buffer stage_buffer = vkb::Buffer::create_staging_buffer(device, indices);

        mesh_data.index_buffer = std::make_unique<vkb::Buffer>(device,
                                                               indices.size(),
                                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                               VMA_MEMORY_USAGE_GPU_ONLY);

        command_buffer->copy_buffer(stage_buffer, *mesh_data.index_buffer, indices.size());

        command_buffer->end();

        queue.submit(*command_buffer, device.request_fence());

        device.get_fence_pool().wait();
        device.get_fence_pool().reset();
        device.get_command_pool().reset_pool();
    }
}
