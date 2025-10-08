/* Copyright (c) 2018-2019, Arm Limited and Contributors
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
#include <vector>

#include "SceneGraph/Component.h"
#include "SceneGraph/Components/AABB.h"

/**
 * @brief The structure of a meshlet for mesh shader
 */
struct Meshlet
{
    uint32_t vertices[64];
    uint32_t indices[126];
    uint32_t vertex_count;
    uint32_t index_count;
};

/**
 * @brief The structure of a vertex
 */
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 joint0;
    glm::vec4 weight0;
    glm::vec3 color;
};

/**
 * @brief The structure of a vertex for storage buffer
 * Simplified to position and normal for easier alignment
 */
struct AlignedVertex
{
    glm::vec4 pos;
    glm::vec4 normal;
};

namespace vkb
{
    namespace sg
    {
        class SubMesh;

        class Mesh : public Component
        {
        public:
            Mesh(const std::string& name);

            virtual ~Mesh() = default;

            void update_bounds(const std::vector<glm::vec3>& vertex_data, const std::vector<uint16_t>& index_data = {});

            virtual std::type_index get_type() override;

            const AABB& get_bounds() const;

            void add_submesh(SubMesh& submesh);

            const std::vector<SubMesh*>& get_submeshes() const;

            void add_node(Node& node);

            const std::vector<Node*>& get_nodes() const;

        private:
            AABB bounds;

            std::vector<SubMesh*> submeshes;

            std::vector<Node*> nodes;
        };
    } // namespace sg
} // namespace vkb
