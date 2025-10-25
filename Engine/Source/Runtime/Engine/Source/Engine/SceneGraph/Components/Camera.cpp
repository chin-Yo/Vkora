/* Copyright (c) 2019-2020, Arm Limited and Contributors
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


#include "Engine/SceneGraph/Components/Camera.hpp"

#include "Engine/SceneGraph/Node.hpp"

namespace scene
{
    Camera::Camera(const std::string& name) :
        Component{name}
    {
    }

    glm::mat4 Camera::GetView()
    {
        if (!owner)
        {
            LOG_ERROR("Camera component {} is not attached to a node", GetName());
        }

        auto& transform = owner->GetTransform();
        return glm::inverse(transform.GetWorldMatrix());
    }

    const glm::mat4 Camera::GetPreRotation()
    {
        return preRotation;
    }

    void Camera::SetPreRotation(const glm::mat4& pr)
    {
        preRotation = pr;
    }
}
