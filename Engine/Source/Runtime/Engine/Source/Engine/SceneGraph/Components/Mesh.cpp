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


#include "Engine/SceneGraph/Components/Mesh.hpp"


namespace scene
{
    Mesh::Mesh(const std::string& name) :
        Component{name}
    {
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : Component(std::move(other))
          , submeshes(std::move(other.submeshes))
    {
        SetName(other.GetName());
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this != &other)
        {
            SetName(other.GetName());
            submeshes = std::move(other.submeshes);
        }
        return *this;
    }

    void Mesh::SetSubmesh(SubMesh& submesh)
    {
        submeshes.push_back(&submesh);
    }

    const std::vector<SubMesh*>& Mesh::GetSubmeshes() const
    {
        return submeshes;
    }
}
