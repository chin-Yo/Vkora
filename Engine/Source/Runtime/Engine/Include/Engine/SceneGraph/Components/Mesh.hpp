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
#include <glm/glm.hpp>
#include "Engine/SceneGraph/Component.hpp"

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

namespace scene
{
    class SubMesh;

    class Mesh : public Component
    {
    public:
        Mesh(const std::string& name);
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;
        virtual ~Mesh() = default;

        void SetSubmesh(SubMesh& submesh);

        const std::vector<SubMesh*>& GetSubmeshes() const;

    private:
        std::vector<SubMesh*> submeshes;
    };
}
