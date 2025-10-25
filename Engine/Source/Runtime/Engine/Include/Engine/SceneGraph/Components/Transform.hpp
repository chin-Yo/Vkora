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

#pragma once

#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

#include "Engine/SceneGraph/Component.hpp"


namespace scene
{
    class Node;

    class Transform : public Component
    {
        RTTR_ENABLE(Component)
    public:
        Transform(Node& node);

        virtual ~Transform() = default;

        Node& GetNode();

        void SetTranslation(const glm::vec3& translation);

        void SetRotation(const glm::quat& rotation);

        void SetScale(const glm::vec3& scale);

        const glm::vec3& GetTranslation() const;

        const glm::quat& GetRotation() const;

        const glm::vec3& GetScale() const;

        void SetMatrix(const glm::mat4& matrix);

        glm::mat4 GetMatrix() const;

        glm::mat4 GetWorldMatrix();

        /**
         * @brief Marks the world transform invalid if any of
         *        the local transform are changed or the parent
         *        world transform has changed.
         */
        void InvalidateWorldMatrix();

        static glm::vec3 QuatToEulerDegrees(const glm::quat& q);
        static glm::quat EulerDegreesToQuat(const glm::vec3& eulerDegrees);

    private:
        Node& node;

        glm::vec3 translation = glm::vec3(0.0, 0.0, 0.0);

        glm::quat rotation = glm::quat(1.0, 0.0, 0.0, 0.0);

        glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);

        glm::mat4 world_matrix = glm::mat4(1.0);

        bool update_world_matrix = false;

        void UpdateWorldTransform();
    };
} // namespace sg
