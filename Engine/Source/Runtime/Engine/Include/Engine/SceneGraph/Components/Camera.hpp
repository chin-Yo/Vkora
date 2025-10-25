/* Copyright (c) 2019-2024, Arm Limited and Contributors
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


#include <string>

#include "Engine/SceneGraph/Component.hpp"
#include "Framework/Common/VkError.hpp"
#include "Framework/Common/glmCommon.hpp"
#include "Framework/Common/VkHelpers.hpp"

namespace scene
{
    class Camera : public Component
    {
    public:
        Camera(const std::string& name);

        virtual ~Camera() = default;

        virtual glm::mat4 GetProjection() = 0;

        glm::mat4 GetView();

        const glm::mat4 GetPreRotation();

        void SetPreRotation(const glm::mat4& pre_rotation);

    private:
        glm::mat4 preRotation{1.0f};
    };
}
