/* Copyright (c) 2018-2025, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Engine/SceneGraph/Component.hpp"
#include "Framework/Common/VkCommon.hpp"

#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/ShaderModule.hpp"


namespace scene
{
    class Material;

    struct VertexAttribute
    {
        VkFormat format = VK_FORMAT_UNDEFINED;

        std::uint32_t stride = 0;

        std::uint32_t offset = 0;
    };

    struct MeshData
    {
        struct VertexBufferBinding
        {
            vkb::Buffer* buffer = nullptr;
            std::uint32_t stride = 0;
            VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
        };

        struct VertexAttribute
        {
            VkFormat format = VK_FORMAT_UNDEFINED;
            std::uint32_t offset = 0;
            std::string binding_name;
        };

        std::uint32_t vertices_count = 0;
        std::uint32_t index_count = 0;
        VkIndexType index_type = VK_INDEX_TYPE_UINT32;

        std::unordered_map<std::string, vkb::Buffer> vertex_buffers;

        std::unique_ptr<vkb::Buffer> index_buffer;
        std::uint32_t index_buffer_offset = 0;

        std::unordered_map<std::string, MeshData::VertexBufferBinding> vertex_buffer_bindings;

        // "Position" : 
        std::unordered_map<std::string, MeshData::VertexAttribute> vertex_attributes;
    };

    class SubMesh : public Component
    {
    public:
        SubMesh(const std::string& name = {});

        virtual ~SubMesh() = default;

        SubMesh(SubMesh&& other) noexcept;

        SubMesh& operator=(SubMesh&& other) noexcept;

        RTTR_ENABLE(Component)
    public:
        MeshData* meshData = nullptr;
        std::string ModelPath;
        bool bHasMeshData = false;

        bool GetAttribute(const std::string& name, MeshData::VertexAttribute& attribute) const;

        VkIndexType index_type{};

        std::uint32_t index_buffer_offset = 0;

        std::uint32_t vertices_count = 0;

        std::uint32_t index_count = 0;

        std::unordered_map<std::string, vkb::Buffer> vertex_buffers;

        std::unique_ptr<vkb::Buffer> index_buffer;

        void set_attribute(const std::string& name, const VertexAttribute& attribute);

        bool get_attribute(const std::string& name, VertexAttribute& attribute) const;

        void set_material(const Material& material);

        const Material* get_material() const;

        const vkb::ShaderVariant& get_shader_variant() const;

        vkb::ShaderVariant& get_mut_shader_variant();

    private:
        std::unordered_map<std::string, VertexAttribute> vertex_attributes;

        const Material* material{nullptr};

        vkb::ShaderVariant shader_variant;
    };
}
