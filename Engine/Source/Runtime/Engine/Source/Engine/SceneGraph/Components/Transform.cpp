/* Copyright (c) 2018-2024, Arm Limited and Contributors
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


#include "Engine/SceneGraph/Components/Transform.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include "Engine/SceneGraph/Node.hpp"

namespace scene
{
    Transform::Transform(Node& n) :
        node{n}
    {
    }

    Node& Transform::GetNode()
    {
        return node;
    }

    void Transform::SetTranslation(const glm::vec3& new_translation)
    {
        translation = new_translation;

        InvalidateWorldMatrix();
    }

    void Transform::SetRotation(const glm::quat& new_rotation)
    {
        rotation = new_rotation;

        InvalidateWorldMatrix();
    }

    void Transform::SetScale(const glm::vec3& new_scale)
    {
        scale = new_scale;

        InvalidateWorldMatrix();
    }

    const glm::vec3& Transform::GetTranslation() const
    {
        return translation;
    }

    const glm::quat& Transform::GetRotation() const
    {
        return rotation;
    }

    const glm::vec3& Transform::GetScale() const
    {
        return scale;
    }

    void Transform::SetMatrix(const glm::mat4& matrix)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);

        InvalidateWorldMatrix();
    }

    glm::mat4 Transform::GetMatrix() const
    {
        return glm::translate(glm::mat4(1.0), translation) *
            glm::mat4_cast(rotation) *
            glm::scale(glm::mat4(1.0), scale);
    }

    glm::mat4 Transform::GetWorldMatrix()
    {
        UpdateWorldTransform();

        return world_matrix;
    }

    void Transform::InvalidateWorldMatrix()
    {
        update_world_matrix = true;
    }

    // Helper: quat -> euler (degrees)
    glm::vec3 Transform::QuatToEulerDegrees(const glm::quat& q)
    {
        glm::vec3 eulerAngles = glm::eulerAngles(q); // returns radians
        return glm::degrees(eulerAngles);
    }

    // Helper: euler (degrees) -> quat
    glm::quat Transform::EulerDegreesToQuat(const glm::vec3& eulerDegrees)
    {
        glm::vec3 radians = glm::radians(eulerDegrees);
        return glm::quat(glm::vec3(radians.x, radians.y, radians.z));
    }

    void Transform::UpdateWorldTransform()
    {
        if (!update_world_matrix)
        {
            return;
        }

        world_matrix = GetMatrix();

        auto parent = node.GetParent();

        if (parent)
        {
            auto& transform = parent->GetTransform();
            world_matrix = transform.GetWorldMatrix() * world_matrix;
        }

        update_world_matrix = false;
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    using namespace scene;
    registration::class_<Transform>("TransformComponent")
        .constructor<Node&>()
        .property("translation", &Transform::GetTranslation, &Transform::SetTranslation)
        .property("rotation", &Transform::GetRotation, &Transform::SetRotation)
        .property("scale", &Transform::GetScale, &Transform::SetScale)

        .method("SetMatrix", &Transform::SetMatrix)
        .method("GetMatrix", &Transform::GetMatrix)
        .method("GetWorldMatrix", &Transform::GetWorldMatrix)
        .method("InvalidateWorldMatrix", &Transform::InvalidateWorldMatrix);
}
