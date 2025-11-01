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


#include "Engine/SceneGraph/Components/SubMesh.hpp"


namespace scene
{
    SubMesh::SubMesh(SubMesh&& other) noexcept
        : Component(std::move(other)),
          ModelPath(std::move(other.ModelPath)),
          bHasMeshData(other.bHasMeshData),
          index_type(other.index_type),
          index_buffer_offset(other.index_buffer_offset),
          vertices_count(other.vertices_count),
          index_count(other.index_count),
          vertex_buffers(std::move(other.vertex_buffers)),
          index_buffer(std::move(other.index_buffer)),
          vertex_attributes(std::move(other.vertex_attributes)),
          material(other.material),
          shader_variant(std::move(other.shader_variant))
    {
    }

    SubMesh& SubMesh::operator=(SubMesh&& other) noexcept
    {
        if (this == &other)
            return *this;
        Component::operator =(std::move(other));
        ModelPath = std::move(other.ModelPath);
        bHasMeshData = other.bHasMeshData;
        index_type = other.index_type;
        index_buffer_offset = other.index_buffer_offset;
        vertices_count = other.vertices_count;
        index_count = other.index_count;
        vertex_buffers = std::move(other.vertex_buffers);
        index_buffer = std::move(other.index_buffer);
        vertex_attributes = std::move(other.vertex_attributes);
        material = other.material;
        shader_variant = std::move(other.shader_variant);
        return *this;
    }

    bool SubMesh::GetAttribute(const std::string& attributeName, MeshData::VertexAttribute& attribute) const
    {
        if (!meshData) return false;

        auto attrib_it = meshData->vertex_attributes.find(attributeName);

        if (attrib_it == meshData->vertex_attributes.end())
        {
            return false;
        }

        attribute = attrib_it->second;

        return true;
    }

    SubMesh::SubMesh(const std::string& name) :
        Component{name}
    {
    }

    void SubMesh::set_attribute(const std::string& attribute_name, const VertexAttribute& attribute)
    {
        vertex_attributes[attribute_name] = attribute;
    }

    bool SubMesh::get_attribute(const std::string& attribute_name, VertexAttribute& attribute) const
    {
        auto attrib_it = vertex_attributes.find(attribute_name);

        if (attrib_it == vertex_attributes.end())
        {
            return false;
        }

        attribute = attrib_it->second;

        return true;
    }

    void SubMesh::set_material(const Material& new_material)
    {
        material = &new_material;
    }

    const Material* SubMesh::get_material() const
    {
        return material;
    }

    const vkb::ShaderVariant& SubMesh::get_shader_variant() const
    {
        return shader_variant;
    }

    vkb::ShaderVariant& SubMesh::get_mut_shader_variant()
    {
        return shader_variant;
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<scene::SubMesh>("scene::SubMesh")
        .constructor<const std::string&>();
}
